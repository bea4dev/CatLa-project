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

    private:
        void collect_cycles();

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

inline void decrement_reference_count(CycleCollector* cycle_collector, HeapObject* object) {
    size_t previous = object->count.fetch_sub(1, std::memory_order_release);
    atomic_thread_fence(std::memory_order_acquire);

    if (previous == 1) {
        stack<HeapObject*> check_object;
        vector<HeapObject*> release_objects;
        auto* current_object = object;
        bool async_release;
        while (true) {
            auto** fields = (HeapObject**) (object + 1);
            size_t field_length = object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    size_t field_object_previous_rc = field_object->count.fetch_sub(1, std::memory_order_release);
                    atomic_thread_fence(std::memory_order_acquire);
                    if (field_object_previous_rc == 1) {

                    }
                }
            }
        }
    }
}
