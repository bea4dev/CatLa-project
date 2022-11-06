#pragma once

#include <heap/HeapAllocator.h>
#include <stack>
#include <util/Concurrent.h>
#include <vector>
#include <vm/modules/Type.h>

using namespace std;
using namespace concurrent;
using namespace modules;

namespace gc {
    class CycleCollector {
    private:
        void* vm;
        SpinLock list_lock;
        vector<HeapObject*>* suspected_object_list;
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
    auto* type = (Type*) object->type_info;
    if (type->is_cycling_type.load(std::memory_order_acquire)) {
        //Possibly, leaking objects will occur during dynamic loading.(?)
        cycle_collector->collect_lock.read_lock();
        size_t previous_count = object->count.fetch_sub(1, std::memory_order_release);
        atomic_thread_fence(std::memory_order_acquire);

        if (previous_count != 1) {
            //Suspect cycles
            size_t expected = 1;
            if (object->flag.compare_exchange_strong(expected, 2)) {
                cycle_collector->add_suspected_object(object);
            }
            cycle_collector->collect_lock.read_unlock();
            return;
        }

        auto* current_object = object;
        stack<HeapObject*> remove_objects;
        while (true) {
            auto* current_object_type = (Type*) current_object->type_info;
            auto** field_objects = (HeapObject**) (object + 1);
            size_t field_length = current_object->field_length;

            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = field_objects[i];
                size_t field_object_previous_count = field_object->count.fetch_sub(1, std::memory_order_release);
                atomic_thread_fence(std::memory_order_acquire);

                if (field_object_previous_count != 1) {
                    auto* field_type = (Type*) field_object->type_info;
                    if (field_type->is_cycling_type.load(std::memory_order_acquire)) {
                        //Suspect cycles
                        size_t expected2 = 1;
                        if (field_object->flag.compare_exchange_strong(expected2, 2)) {
                            cycle_collector->add_suspected_object(field_object);
                        }
                    }
                    continue;
                }
                remove_objects.push(field_object);
            }

            atomic_thread_fence(std::memory_order_release);
            if (current_object_type->is_cycling_type.load(std::memory_order_acquire)) {
                atomic_thread_fence(std::memory_order_seq_cst);
                size_t flag_old = current_object->flag.exchange(3, std::memory_order_acquire);
                if (flag_old == 1) {
                    cycle_collector->add_suspected_object(current_object);
                }
            } else {
                //Release object
                current_object->flag.store(0, std::memory_order_release);
                atomic_thread_fence(std::memory_order_acquire);
            }

            if (remove_objects.empty()) {
                break;
            }
            current_object = remove_objects.top();
            remove_objects.pop();
        }
        cycle_collector->collect_lock.read_unlock();
    } else {
        size_t previous_count = object->count.fetch_sub(1, std::memory_order_release);
        atomic_thread_fence(std::memory_order_acquire);
        if (previous_count != 1) {
            return;
        }

        auto *current_object = object;
        stack<HeapObject*> remove_objects;
        while (true) {
            auto** field_objects = (HeapObject**) (object + 1);
            size_t field_length = current_object->field_length;

            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = field_objects[i];
                size_t field_object_previous_count = field_object->count.fetch_sub(1, std::memory_order_release);
                atomic_thread_fence(std::memory_order_acquire);

                if (field_object_previous_count != 1) {
                    auto* field_type = (Type*) field_object->type_info;
                    if (field_type->is_cycling_type.load(std::memory_order_acquire)) {
                        //Suspect cycles
                        size_t expected2 = 1;
                        if (field_object->flag.compare_exchange_strong(expected2, 2)) {
                            cycle_collector->add_suspected_object(field_object);
                        }
                    }
                    continue;
                }
                remove_objects.push(field_object);
            }

            //Release object
            current_object->flag.store(0, std::memory_order_release);
            atomic_thread_fence(std::memory_order_acquire);

            if (remove_objects.empty()) {
                break;
            }
            current_object = remove_objects.top();
            remove_objects.pop();
        }
    }
}
