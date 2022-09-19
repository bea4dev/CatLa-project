#pragma once
#include <cstdint>
#include <util/Concurrent.h>

using namespace concurrent;

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


class HeapChunk {
private:
    size_t cells_size;
    uint8_t** entry_positions;
    bool* block_space_info;

public:
    explicit HeapChunk(size_t cells_size);
    ~HeapChunk();
    void* alloc_for_class(CatLaClass* class_info, size_t refs_length, size_t vals_length);
    void* malloc(CatLaClass* class_info, size_t index, size_t block_size, size_t* byte_size);
};

class GlobalHeap {
private:
    size_t chunks_size;
    HeapChunk** chunks;
    SpinLock lock;

public:
    explicit GlobalHeap(size_t chunks_cells_size);
    void* malloc(CatLaClass* class_info, size_t refs_length, size_t vals_length, size_t* start_index);
    void create_new_chunk(size_t cells_size);
};