#pragma once
#include <cstdint>
#include <util/Concurrent.h>

using namespace concurrent;

enum object_state : uint8_t {
    dead,
    live,
    process_gc,
    waiting_for_gc,
};

typedef struct {
    atomic_size_t count;
    size_t crc;
    atomic_uint8_t state;
    atomic_bool async_release;
    bool is_cyclic_type;
    atomic_flag lock_flag;
    size_t field_length;
    void* type_info;
} HeapObject;

inline void object_lock(HeapObject* object) {
    while (object->lock_flag.test_and_set(std::memory_order_acquire)) {
        //wait
    }
}

inline void object_unlock(HeapObject* object) {
    object->lock_flag.clear(std::memory_order_release);
}

inline uint64_t get_object_field(HeapObject* object, size_t field_index) {
    auto** field_ptr = ((HeapObject**) (object + 1)) + field_index;
    return (uint64_t) *field_ptr;
}

inline void set_object_field(HeapObject* object, size_t field_index, uint64_t value) {
    auto** field_ptr = ((HeapObject**) (object + 1)) + field_index;
    *field_ptr = (HeapObject*) value;
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
