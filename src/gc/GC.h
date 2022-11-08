#pragma once

#include <heap/HeapAllocator.h>
#include <stack>
#include <util/Concurrent.h>
#include <vector>
#include <vm/modules/Type.h>
#include <vm/modules/util/BitSet.h>
#include <pthread.h>

using namespace std;
using namespace concurrent;
using namespace modules;

namespace gc {
    class CycleCollector {
    private:
        void* vm;
        SpinLock list_lock;
        vector<HeapObject*>* suspected_object_list;
        pthread_mutex_t collector_lock;

    public:
        RWLock collect_lock;

    public:
        explicit CycleCollector(void* vm);
        inline void add_suspected_object(HeapObject* object) {
            list_lock.lock();
            suspected_object_list->push_back(object);
            list_lock.unlock();
        }
        void collect_cycles();
    };
}

using namespace gc;

inline HeapObject* clone_object_field_ownership(HeapObject* parent_object, size_t field_index) {
    auto* fields = (uint64_t*) (parent_object + 1);
    while (true) {
        object_lock(parent_object);
        auto* field_object = (HeapObject*) fields[field_index];

        if (field_object == (HeapObject*) 1) {
            object_unlock(parent_object);
            continue;
        }

        field_object->count.fetch_add(1, std::memory_order_relaxed);
        object_unlock(parent_object);
        return field_object;
    }
}

inline HeapObject* move_object_field_ownership(HeapObject* parent_object, size_t field_index) {
    auto* fields = (uint64_t*) (parent_object + 1);
    while (true) {
        object_lock(parent_object);
        auto* field_object = (HeapObject*) fields[field_index];

        if (field_object == (HeapObject*) 1) {
            object_unlock(parent_object);
            continue;
        }

        fields[field_index] = 0;
        object_unlock(parent_object);
        return field_object;
    }
}

inline void increase_reference_count(HeapObject* object) {
    object->count.fetch_add(1, std::memory_order_relaxed);
}

inline void decrease_reference_count(CycleCollector* cycle_collector, HeapObject* object) {
    auto* current_object = object;
    bool cycling_lock = false;
    stack<HeapObject*> check_objects;

    while (true) {
        auto* current_object_type = (Type*) current_object->type_info;

        //Possibly, leaking objects will occur on dynamic load.(?)
        //But, it's safe.
        //Improve it in the future.
        bool is_cycling_type = current_object_type->is_cycling_type.load(std::memory_order_acquire);
        if (is_cycling_type) {
            if (!cycling_lock) {
                //Temporarily stops the operation of Cycle Collector.
                cycle_collector->collect_lock.read_lock();
                cycling_lock = true;
            }
        }

        size_t previous_count = current_object->count.fetch_sub(1, std::memory_order_release);
        atomic_thread_fence(std::memory_order_acquire);

        if (is_cycling_type) {
            if (previous_count == 1) {
                size_t previous_flag = current_object->flag.exchange(3, std::memory_order_acquire);
                if (previous_flag == 3) {
                    //Probably won't happen.
                    if (check_objects.empty()) {
                        break;
                    }
                    current_object = check_objects.top();
                    check_objects.pop();
                    continue;
                } else if (previous_flag != 2) {
                    //printf("previous_flag == %llu\n", previous_count);
                    cycle_collector->add_suspected_object(current_object);
                }
            }

            if (previous_count != 1) {
                size_t expected = 1;
                if (current_object->flag.compare_exchange_strong(expected, 2)) {
                    cycle_collector->add_suspected_object(current_object);
                }

                if (check_objects.empty()) {
                    break;
                }
                current_object = check_objects.top();
                check_objects.pop();
                continue;
            }
        } else {
            if (previous_count != 1) {
                if (check_objects.empty()) {
                    break;
                }
                current_object = check_objects.top();
                check_objects.pop();
                continue;
            }
        }

        //Collect field objects.
        size_t field_length = current_object->field_length;
        auto** fields = (HeapObject**) (current_object + 1);
        for (size_t i = 0; i < field_length; i++) {
            if (get_flag(current_object_type->reference_fields, i)) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    check_objects.push(field_object);
                }
            }
        }

        if (!is_cycling_type) {
            //Release object.
            current_object->flag.store(0, std::memory_order_release);
        }

        if (check_objects.empty()) {
            break;
        }
        current_object = check_objects.top();
        check_objects.pop();
    }

    if (cycling_lock) {
        cycle_collector->collect_lock.read_unlock();
    }
}
