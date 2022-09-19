#include "HeapChunk.h"
#include <util/Benchmark.h>

using namespace heap;


#define BLOCK_SIZE0 32
#define BLOCK_SIZE1 40
#define BLOCK_SIZE2 48
#define BLOCK_SIZE3 56
#define BLOCK_SIZE4 64
#define BLOCK_SIZE5 72
#define BLOCK_SIZE6 80
#define BLOCK_SIZE7 88
#define BLOCK_SIZE8 96
#define BLOCK_SIZE9 128
#define BLOCK_SIZE10 256
#define BLOCK_SIZE11 384
#define BLOCK_SIZE12 512
#define BLOCK_SIZE13 1024
#define BLOCK_SIZE14 2048
#define BLOCK_SIZE15 3072
#define BLOCK_SIZE16 4096

#define NUMBER_OF_BLOCKS 17
#define BLOCKS_SIZE 12096


HeapChunk::HeapChunk(size_t cells_size) {
    this->cells_size = cells_size;

    size_t entry_positions_size = NUMBER_OF_BLOCKS * sizeof(void*);
    auto* position0 = (uint8_t*) calloc(1, cells_size * BLOCKS_SIZE + entry_positions_size);

    auto* position1 = position0 + BLOCK_SIZE0 * cells_size;
    auto* position2 = position1 + BLOCK_SIZE1 * cells_size;
    auto* position3 = position2 + BLOCK_SIZE2 * cells_size;
    auto* position4 = position3 + BLOCK_SIZE3 * cells_size;
    auto* position5 = position4 + BLOCK_SIZE4 * cells_size;
    auto* position6 = position5 + BLOCK_SIZE5 * cells_size;
    auto* position7 = position6 + BLOCK_SIZE6 * cells_size;
    auto* position8 = position7 + BLOCK_SIZE7 * cells_size;
    auto* position9 = position8 + BLOCK_SIZE8 * cells_size;
    auto* position10 = position9 + BLOCK_SIZE9 * cells_size;
    auto* position11 = position10 + BLOCK_SIZE10 * cells_size;
    auto* position12 = position11 + BLOCK_SIZE11 * cells_size;
    auto* position13 = position12 + BLOCK_SIZE12 * cells_size;
    auto* position14 = position13 + BLOCK_SIZE13 * cells_size;
    auto* position15 = position14 + BLOCK_SIZE14 * cells_size;
    auto* position16 = position15 + BLOCK_SIZE15 * cells_size;
    this->entry_positions = (uint8_t**) (position16 + BLOCK_SIZE16 * cells_size);

    entry_positions[0] = position0;
    entry_positions[1] = position1;
    entry_positions[2] = position2;
    entry_positions[3] = position3;
    entry_positions[4] = position4;
    entry_positions[5] = position5;
    entry_positions[6] = position6;
    entry_positions[7] = position7;
    entry_positions[8] = position8;
    entry_positions[9] = position9;
    entry_positions[10] = position10;
    entry_positions[11] = position11;
    entry_positions[12] = position12;
    entry_positions[13] = position13;
    entry_positions[14] = position14;
    entry_positions[15] = position15;
    entry_positions[16] = position16;

    this->block_space_info = new bool[NUMBER_OF_BLOCKS];
    for (int i = 0; i < NUMBER_OF_BLOCKS; i++) {
        this->block_space_info[i] = true;
    }
}


HeapChunk::~HeapChunk() {
    free((void*) this->entry_positions[0]);
}


#if SIZE_MAX == UINT32_MAX
    #define ADDRESS_SIZE_SHIFT 2
#elif SIZE_MAX == UINT64_MAX
    #define ADDRESS_SIZE_SHIFT 3
#endif

void* HeapChunk::malloc(CatLaClass *class_info, size_t index, size_t block_size, size_t* byte_size) {
    if (!this->block_space_info[index]) {
        if (block_size < BLOCK_SIZE8) {
            *byte_size = block_size + 8;
        } else if (block_size == BLOCK_SIZE8) {
            *byte_size = BLOCK_SIZE9;
        } else if (block_size < BLOCK_SIZE12) {
            *byte_size = block_size + 128;
        } else {
            return nullptr;
        }
        return nullptr;
    }

    size_t cells_size_ = this->cells_size;
    uint8_t* block_entry = this->entry_positions[index];

    for (size_t address_index = 0; address_index < cells_size_; address_index++) {
        uint8_t* entry = block_entry + (address_index * block_size);
        auto* object = (HeapObject*) entry;
        if (object->flags != 0) {
            continue;
        }

        object_lock(object);
        if (object->flags != 0) {
            object_unlock(object);
            continue;
        }
        mark_object_alive(object);
        object_unlock(object);
        return object;
    }

    if (block_size < BLOCK_SIZE8) {
        *byte_size = block_size + 8;
    } else if (block_size == BLOCK_SIZE8) {
        *byte_size = BLOCK_SIZE9;
    } else if (block_size < BLOCK_SIZE12) {
        *byte_size = block_size + 128;
    } else {
        return nullptr;
    }

    return nullptr;
}

void* HeapChunk::alloc_for_class(CatLaClass* class_info, size_t refs_length, size_t vals_length) {
    size_t byte_size = sizeof(HeapObject) + (refs_length << ADDRESS_SIZE_SHIFT) + (vals_length << ADDRESS_SIZE_SHIFT);

    size_t index;
    size_t block_size;
    uint8_t* block_entry;
    if (byte_size < BLOCK_SIZE8) {
        if (byte_size <= BLOCK_SIZE0) {
            index = 0;
        } else {
            size_t temp = byte_size - BLOCK_SIZE0;
            if ((temp & 0b111) == 0) {
                index = ((temp) >> 3);
            } else {
                index = ((temp) >> 3) + 1;
            }
        }
        if (!this->block_space_info[index]) {
            return nullptr;
        }
        block_size = BLOCK_SIZE0 + (index << 3);
        block_entry = this->entry_positions[index];
    } else if (byte_size < BLOCK_SIZE12) {
        if (byte_size <= BLOCK_SIZE9) {
            index = 0;
        } else {
            size_t temp = byte_size - BLOCK_SIZE9;
            if ((temp & 0b1111111) == 0) {
                index = ((temp) >> 7);
            } else {
                index = ((temp) >> 7) + 1;
            }
        }
        if (!this->block_space_info[index + 9]) {
            return nullptr;
        }
        block_size = BLOCK_SIZE9 + (index << 7);
        block_entry = this->entry_positions[index + 9];
    } else {
        if (byte_size <= BLOCK_SIZE13) {
            index = 0;
        } else {
            size_t temp = byte_size - BLOCK_SIZE13;
            if ((temp & 0b1111111111111) == 0) {
                index = ((temp) >> 10);
            } else {
                index = ((temp) >> 10) + 1;
            }
        }
        if (!this->block_space_info[index + 13]) {
            return nullptr;
        }
        block_size = BLOCK_SIZE13 + (index << 10);
        block_entry = this->entry_positions[index + 13];
    }

    size_t cells_size_ = this->cells_size;
    for (size_t address_index = 0; address_index < cells_size_; address_index++) {
        uint8_t* entry = block_entry + (address_index * block_size);
        auto* object = (HeapObject*) entry;
        if (object->flags != 0) {
            continue;
        }

        object_lock(object);
        if (object->flags != 0) {
            object_unlock(object);
            continue;
        }
        mark_object_alive(object);
        object_unlock(object);
        return object;
    }

    return nullptr;
}


GlobalHeap::GlobalHeap(size_t chunks_cells_size) {
    this->chunks_size = 0;
    this->chunks = new HeapChunk*[0];
    create_new_chunk(chunks_cells_size);
}

void* GlobalHeap::malloc(CatLaClass* class_info, size_t refs_length, size_t vals_length, size_t* start_index) {
    size_t byte_size = sizeof(HeapObject) + (refs_length << ADDRESS_SIZE_SHIFT) + (vals_length << ADDRESS_SIZE_SHIFT);

    size_t index;
    size_t block_size;
    if (byte_size < BLOCK_SIZE8) {
        if (byte_size <= BLOCK_SIZE0) {
            index = 0;
        } else {
            size_t temp = byte_size - BLOCK_SIZE0;
            if ((temp & 0b111) == 0) {
                index = ((temp) >> 3);
            } else {
                index = ((temp) >> 3) + 1;
            }
        }
        block_size = BLOCK_SIZE0 + (index << 3);
    } else if (byte_size < BLOCK_SIZE12) {
        size_t index_temp;
        if (byte_size <= BLOCK_SIZE9) {
            index_temp = 0;
        } else {
            size_t temp = byte_size - BLOCK_SIZE9;
            if ((temp & 0b1111111) == 0) {
                index_temp = ((temp) >> 7);
            } else {
                index_temp = ((temp) >> 7) + 1;
            }
        }
        block_size = BLOCK_SIZE9 + (index_temp << 7);
        index = index_temp + 9;
    } else {
        size_t index_temp;
        if (byte_size <= BLOCK_SIZE13) {
            index_temp = 0;
        } else {
            size_t temp = byte_size - BLOCK_SIZE13;
            if ((temp & 0b1111111111111) == 0) {
                index_temp = ((temp) >> 10);
            } else {
                index_temp = ((temp) >> 10) + 1;
            }
        }
        block_size = BLOCK_SIZE13 + (index_temp << 10);
        index = index_temp + 13;
    }

    size_t current_chunk_index = *start_index;

    this->lock.lock();
    HeapChunk **heap_chunks = this->chunks;
    size_t cs = this->chunks_size;
    this->lock.unlock();

    for (size_t i = 0; i < cs; i++) {
        HeapChunk *chunk = heap_chunks[current_chunk_index];
        size_t size_temp = byte_size;

        while (true) {
            printf("%llu\n", size_temp);
            void *address = chunk->malloc(class_info, index, block_size, &size_temp);
            if (address != nullptr) {
                *start_index = current_chunk_index;
                return address;
            }

            if (size_temp == BLOCK_SIZE12) {
                printf("BREAK!\n");
                break;
            }
        }

        current_chunk_index++;
        if (current_chunk_index == cs) {
            current_chunk_index = 0;
        }
    }

    return nullptr;
}

void GlobalHeap::create_new_chunk(size_t cells_size) {
    auto* chunk = new HeapChunk(cells_size);

    this->lock.lock();
    size_t chunks_size_old = this->chunks_size;
    size_t chunks_size_new = chunks_size_old + 1;

    auto** heap_chunks_old = chunks;
    auto** heap_chunks_new = new HeapChunk*[chunks_size_new];
    for (size_t i = 0; i < chunks_size_old; i++) {
        heap_chunks_new[i] = heap_chunks_old[i];
    }
    heap_chunks_new[chunks_size_old] = chunk;

    this->chunks = heap_chunks_new;
    this->chunks_size = chunks_size_new;
    this->lock.unlock();
}


