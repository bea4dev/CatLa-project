#pragma once

#include <heap/HeapAllocator.h>
#include <stack>
#include <util/Concurrent.h>
#include <vector>
#include <vm/modules/Type.h>
#include <vm/modules/util/BitSet.h>
#include <pthread.h>
#include <unordered_set>

using namespace std;
using namespace concurrent;
using namespace modules;

namespace gc {
    class CycleCollector {
    public:
        void* vm;
        SpinLock list_lock;
        unordered_set<HeapObject*>* suspected_object_list;
        pthread_mutex_t collector_lock;
        vector<unordered_set<HeapObject*>> cycle_buffer;
        unordered_set<HeapObject*> retry;

    public:
        RWLock collect_lock;
        unordered_set<HeapObject*> suspected;
        unordered_set<HeapObject*> white;
        unordered_set<HeapObject*> gray;

    public:
        explicit CycleCollector(void* vm);
        inline void add_suspected_object(HeapObject* object) {
            list_lock.lock();
            suspected_object_list->insert(object);
            list_lock.unlock();
        }
        void gc_collect();

    };
}

using namespace gc;

inline void increment_reference_count(CycleCollector* cycle_collector, HeapObject* object) {
    size_t previous = object->count.fetch_add(1, std::memory_order_relaxed);
    if (previous == 1 && object->is_cyclic_type) {
        bool expected = false;
        if (object->async_release.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
            cycle_collector->add_suspected_object(object);
        }
    }
}



inline void decrement_reference_count_waiting_gc_object(HeapObject* object) {
    auto* current_root = object;
    stack<HeapObject*> check_roots;
    while (true) {
        vector<HeapObject*> release_objects;
        stack<HeapObject*> check_objects;
        auto* current_object = current_root;
        while (true) {
            release_objects.push_back(current_object);
            current_object->state.store(object_state::process_gc, std::memory_order_release);
            atomic_thread_fence(std::memory_order_acquire);

            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    size_t field_object_previous_rc = field_object->count.fetch_sub(1, std::memory_order_release);
                    atomic_thread_fence(std::memory_order_acquire);
                    if (field_object_previous_rc == 1) {
                        if (field_object->is_cyclic_type && field_object->async_release.load(std::memory_order_acquire)) {
                            fields[i] = nullptr;
                            check_roots.push(field_object);
                        } else {
                            check_objects.push(field_object);
                        }
                    } else {
                        fields[i] = nullptr;
                    }
                }
            }

            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }

        for (auto& obj : release_objects) {
            obj->state.store(object_state::waiting_for_gc, std::memory_order_release);
        }

        if (check_roots.empty()) {
            break;
        }
        current_root = check_roots.top();
        check_roots.pop();
    }
}

inline void decrement_reference_count(CycleCollector* cycle_collector, HeapObject* object) {
    size_t previous = object->count.fetch_sub(1, std::memory_order_release);
    atomic_thread_fence(std::memory_order_acquire);
    if (previous == 0) {
        printf("ZERO! [%p] : %d %d\n", object, object->async_release.load(), object->state.load());
    }
    if (previous == 1) {
        if (object->is_cyclic_type && object->async_release.load(std::memory_order_acquire)) {
            decrement_reference_count_waiting_gc_object(object);
            return;
        }
    } else {
        return;
    }

    stack<HeapObject*> check_objects;
    auto* current_object = object;
    while (true) {
        auto** fields = (HeapObject**) (current_object + 1);
        size_t field_length = current_object->field_length;
        for (size_t i = 0; i < field_length; i++) {
            auto* field_object = fields[i];
            if (field_object != nullptr) {
                size_t field_object_previous_rc = field_object->count.fetch_sub(1, std::memory_order_release);
                if (field_object_previous_rc == 0) {
                    printf("ZERO FIELD! [%p] : %d %d\n", field_object, field_object->async_release.load(), field_object->state.load());
                }
                atomic_thread_fence(std::memory_order_acquire);
                if (field_object_previous_rc == 1) {
                    if (field_object->is_cyclic_type && field_object->async_release.load(std::memory_order_acquire)) {
                        decrement_reference_count_waiting_gc_object(field_object);
                        continue;
                    } else {
                        check_objects.push(field_object);
                    }
                }
            }
        }

        current_object->state.store(object_state::dead, std::memory_order_release);
        if (check_objects.empty()) {
            break;
        }
        current_object = check_objects.top();
        check_objects.pop();
    }
}
