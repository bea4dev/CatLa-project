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
    Type* class_info; //8byte
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


typedef struct {
    uint8_t* entry_position;
    bool empty;
    size_t current_automaton_index;
} BlockInfo;

class HeapChunk {
public:
    size_t cells_size;
    BlockInfo* block_info_list;

public:
    explicit HeapChunk(size_t cells_size);
    ~HeapChunk();
    void* malloc(Type* class_info, size_t index, size_t block_size);
};

class GlobalHeap {
private:
    size_t number_of_chunks;
    HeapChunk** chunks;
    SpinLock lock;
    size_t chunks_cells_size;

public:
    GlobalHeap(size_t chunks_cells_size, size_t number_of_chunks);
    void* malloc(Type* class_info, size_t refs_length, size_t vals_length, size_t* start_index);
    void create_new_chunk(size_t cells_size);
};