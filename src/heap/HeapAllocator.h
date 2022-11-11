#pragma once
#include <cstdint>
#include <util/Concurrent.h>

using namespace concurrent;

typedef struct {
    atomic_size_t count; //8byte
    atomic_size_t flag; //8byte
    size_t lock_flag; //8byte
    size_t field_length; //8byte
    void* type_info; //8byte
} HeapObject;

inline void object_lock(HeapObject* object) {
    auto* flag = (atomic_flag*) &object->lock_flag;
    while (flag->test_and_set(std::memory_order_acquire)) {
        //wait
    }
}

inline void object_unlock(HeapObject* object) {
    auto* flag = (atomic_flag*) &object->lock_flag;
    flag->clear(std::memory_order_release);
}

inline uint64_t get_object_field(HeapObject* object, size_t field_index) {
    auto* ptr = (size_t*) object;
    return ptr[field_index + 5];
}

inline void set_object_field(HeapObject* object, size_t field_index, uint64_t value) {
    auto* ptr = (size_t*) object;
    ptr[field_index + 5] = value;
}

inline void set_object_flag_non_atomic(HeapObject* object, size_t flag) {
    *((size_t*) &object->flag) = flag;
}


typedef struct {
    uint8_t* entry_position;
    bool empty;
    size_t current_location;
} BlockInfo;

class HeapChunk {
public:
    size_t cells_size;
    BlockInfo* block_info_list;

public:
    explicit HeapChunk(size_t cells_size);
    ~HeapChunk();
    void* malloc(void* type_info, size_t index, size_t block_size, size_t field_length, bool is_thread_safe);
};

class HeapAllocator {
private:
    void* vm;
    bool is_thread_safe;
    size_t number_of_chunks;
    HeapChunk** chunks;
    SpinLock lock;
    size_t chunks_cells_size;

public:
    HeapAllocator(void* vm, bool is_thread_safe, size_t chunks_cells_size, size_t number_of_chunks);
    void* malloc(void* type_info, size_t fields_length, size_t* chunk_search_start_index);
    void create_new_chunk(size_t cells_size);
};
