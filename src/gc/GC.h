#pragma once

#include <heap/HeapAllocator.h>
#include <vm/CatVM.h>
#include <stack>

using namespace catla;
using namespace std;

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

inline void decrease_reference_count(HeapObject* object) {
    if (object->count.fetch_sub(1, std::memory_order_release) != 1) {
        return;
    }

    atomic_thread_fence(std::memory_order_acquire);

    auto* current_object = object;
    stack<HeapObject*> remove_objects;
    while (true) {
        auto* field_objects = (HeapObject**) (object + 1);
        size_t field_length = current_object->field_length;

        for (size_t i = 0; i < field_length; i++) {
            auto* field_object = field_objects[i];
            if (field_object->count.fetch_sub(1, std::memory_order_release) != 1) {
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