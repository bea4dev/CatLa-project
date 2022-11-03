#pragma once

#include <heap/HeapAllocator.h>
#include <stack>
#include <util/Concurrent.h>
#include <vector>

using namespace std;
using namespace concurrent;

namespace gc {
    class CycleCollector {
    private:
        void* vm;
        SpinLock list_lock;
        vector<void*>* suspected_object_list;

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
    size_t previous_count = object->count.fetch_sub(1, std::memory_order_release);
    if (previous_count == 2) {
        //Suspect cycles
        size_t expected = 1;
        if (object->flag.compare_exchange_strong(expected, 2, std::memory_order_acquire)) {
            cycle_collector->add_suspected_object(object);
        }
        return;
    } else if (previous_count != 1) {
        return;
    }

    atomic_thread_fence(std::memory_order_acquire);

    auto* current_object = object;
    stack<HeapObject*> remove_objects;
    while (true) {
        //--- flag ---
        //0 : dead
        //1 : live
        //2 : suspected
        //3 : wait for free
        //4~6 : collecting
        size_t expected = 1;
        if (!current_object->flag.compare_exchange_strong(expected, 3, std::memory_order_acquire)) {
            expected = 2;
            if (!current_object->flag.compare_exchange_strong(expected, 3, std::memory_order_acquire)) {
                if (remove_objects.empty()) {
                    break;
                }
                current_object = remove_objects.top();
                remove_objects.pop();
                continue;
            }
        }

        auto** field_objects = (HeapObject**) (object + 1);
        size_t field_length = current_object->field_length;

        for (size_t i = 0; i < field_length; i++) {
            auto* field_object = field_objects[i];
            size_t field_object_previous_count = field_object->count.fetch_sub(1, std::memory_order_release);
            if (field_object_previous_count == 2) {
                //Suspect cycles
                size_t expected2 = 1;
                if (field_object->flag.compare_exchange_strong(expected2, 2, std::memory_order_release)) {
                    cycle_collector->add_suspected_object(field_object);
                }
                continue;
            } else if (field_object_previous_count != 1) {
                continue;
            }
            atomic_thread_fence(std::memory_order_acquire);
            remove_objects.push(field_object);
        }

        if (remove_objects.empty()) {
            break;
        }
        current_object = remove_objects.top();
        remove_objects.pop();
    }
}
