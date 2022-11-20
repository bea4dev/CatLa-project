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
    private:
        void* vm;
        SpinLock list_lock;
        unordered_set<HeapObject*>* suspected_object_list;
        pthread_mutex_t collector_lock;
        vector<unordered_set<HeapObject*>> cycle_buffer;
        vector<HeapObject*> retry;

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
        void process_cycles();

    private:
        void free_cycles(unordered_set<HeapObject*>* roots);
        void collect_cycles(unordered_set<HeapObject*>* roots);
        void sigma_preparation();
        void mark_roots(unordered_set<HeapObject*>* roots);
        void scan_roots(unordered_set<HeapObject*>* roots);
        void scan(HeapObject* s);
        void collect_roots(unordered_set<HeapObject*>* roots);
        void mark_gray(HeapObject* s);
        void collect_white(HeapObject* s, unordered_set<HeapObject*>& current_cycle);
        bool delta_test(unordered_set<HeapObject*>& c);
        bool sigma_test(unordered_set<HeapObject*>& c);
        void free_cycle(unordered_set<HeapObject*>& c);
        void refurbish(unordered_set<HeapObject*>* roots, unordered_set<HeapObject*>& c);
        void cyclic_decrement(HeapObject* m);

    };
}

using namespace gc;

inline void scan_black(HeapObject* object) {
    if (object->color.load(std::memory_order_acquire) == object_color::black) {
        return;
    }
    object->color.store(object_color::black, std::memory_order_release);

    stack<HeapObject*> check_objects;
    auto* current_object = object;
    while (true) {
        auto** fields = (HeapObject**) (current_object + 1);
        size_t field_length = current_object->field_length;

        for (size_t i = 0; i < field_length; i++) {
            auto* field_object = fields[i];
            if (field_object != nullptr) {
                if (field_object->color.load(std::memory_order_acquire) != object_color::black) {
                    field_object->color.store(object_color::black, std::memory_order_release);
                    check_objects.push(field_object);
                }
            }
        }

        if (check_objects.empty()) {
            break;
        }
        current_object = check_objects.top();
        check_objects.pop();
    }
}

namespace gc {
    void possible_roots(CycleCollector* cycle_collector, HeapObject* object);
    void release(CycleCollector* cycle_collector, HeapObject* object);
}

inline void increment_reference_count(HeapObject* object) {
    atomic_thread_fence(std::memory_order_release);
    object->count.fetch_add(1, std::memory_order_acquire);
    scan_black(object);
}

inline void decrement_reference_count(CycleCollector* cycle_collector, HeapObject* object) {
    size_t previous_count = object->count.fetch_sub(1, std::memory_order_release);
    atomic_thread_fence(std::memory_order_acquire);
    if (previous_count == 1) {
        release(cycle_collector, object);
    } else {
        possible_roots(cycle_collector, object);
    }
}
