#pragma once
#include <cstdint>
#include <util/Concurrent.h>


typedef struct {
    atomic_size_t count; //8byte
    uint32_t flags; //4byte
    atomic_flag lock_flag; //4byte
    size_t refs_length; //8byte
    size_t vals_length; //8byte
    CatLaClass* class_info; //8byte
} HeapObject;

inline void object_lock(HeapObject* object) {
    while (object->lock_flag.test_and_set(std::memory_order_acquire)) {
        //wait
    }
}

inline void object_unlock(HeapObject* object) {
    object->lock_flag.clear(std::memory_order_release);
}

inline void mark_object_alive(HeapObject* object) {
    object->flags |= 1;
}


class HeapRegion {
private:
    size_t cells_size;
    uint8_t** entry_positions;

public:
    HeapRegion(size_t cells_size);
    ~HeapRegion();
    void* alloc_for_class(CatLaClass* class_info, size_t refs_length, size_t vals_length);

};