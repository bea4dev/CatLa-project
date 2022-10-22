#include "HeapAllocator.h"
#include <util/Benchmark.h>

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

#define NUMBER_OF_BLOCKS 13
#define BLOCKS_SIZE 1856



void create_heap_chunk(HeapChunk* chunk, size_t cells_size) {
    size_t block_info_list_size = sizeof(BlockInfo) * NUMBER_OF_BLOCKS;
    auto* position0 = (uint8_t*) calloc(1, cells_size * BLOCKS_SIZE + block_info_list_size);

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

    auto* block_info_list = (BlockInfo*) (position12 + BLOCK_SIZE12 * cells_size);

    block_info_list[0].entry_position = position0;
    block_info_list[1].entry_position = position1;
    block_info_list[2].entry_position = position2;
    block_info_list[3].entry_position = position3;
    block_info_list[4].entry_position = position4;
    block_info_list[5].entry_position = position5;
    block_info_list[6].entry_position = position6;
    block_info_list[7].entry_position = position7;
    block_info_list[8].entry_position = position8;
    block_info_list[9].entry_position = position9;
    block_info_list[10].entry_position = position10;
    block_info_list[11].entry_position = position11;
    block_info_list[12].entry_position = position12;

    for (int i = 0; i < NUMBER_OF_BLOCKS; i++) {
        block_info_list[i].empty = true;
    }

    chunk->block_info_list = block_info_list;
    chunk->cells_size = cells_size;
}

HeapChunk::HeapChunk(size_t cells_size) {
    this->cells_size = cells_size;
    this->block_info_list = nullptr;
    this->cells_size = 0;
    create_heap_chunk(this, cells_size);
}


HeapChunk::~HeapChunk() {
    free((void*) this->block_info_list);
}


void* HeapChunk::malloc(void* type_info, size_t index, size_t block_size) {
    size_t cells_size_ = this->cells_size;

    while (true) {
        auto* block_info = (this->block_info_list) + index;

        if (!block_info->empty) {
            index++;
            if (block_size < BLOCK_SIZE8) {
                block_size = block_size + 8;
            } else if (block_size == BLOCK_SIZE8) {
                block_size = BLOCK_SIZE9;
            } else if (block_size <= BLOCK_SIZE12) {
                block_size = block_size + 128;
            } else {
                return nullptr;
            }
            continue;
        }

        uint8_t* block_entry = block_info->entry_position;

        size_t current_entry_index = block_info->current_automaton_index;
        uint8_t* current_entry = block_entry + (current_entry_index * block_size);
        //printf("index : %llu | automaton_index : %llu\n", index, current_entry_index);

        for (size_t i = 0; i < cells_size_; i++) {
            auto* object = (HeapObject*) current_entry;
            if (object->flags != 0) {
                current_entry += block_size;
                current_entry_index++;
                if (current_entry_index == cells_size_) {
                    current_entry = block_entry;
                    current_entry_index = 0;
                }
                continue;
            }

            object_lock(object);
            if (object->flags != 0) {
                object_unlock(object);
                current_entry += block_size;
                current_entry_index++;
                if (current_entry_index == cells_size_) {
                    current_entry = block_entry;
                    current_entry_index = 0;
                }
                continue;
            }
            mark_object_alive(object);
            object_unlock(object);
            //printf("index : %llu, size%llu\n", index, block_size);
            current_entry_index++;
            if (current_entry_index == cells_size_) {
                current_entry_index = 0;
            }
            block_info->current_automaton_index = current_entry_index;

            object->type_info = type_info;

            return object;
        }
        block_info->current_automaton_index = current_entry_index;
        block_info->empty = false;

        index++;
        if (block_size < BLOCK_SIZE8) {
            block_size = block_size + 8;
        } else if (block_size == BLOCK_SIZE8) {
            block_size = BLOCK_SIZE9;
        } else if (block_size <= BLOCK_SIZE12) {
            block_size = block_size + 128;
        } else {
            return nullptr;
        }
    }
}


HeapAllocator::HeapAllocator(size_t chunks_cells_size, size_t number_of_chunks) {
    this->chunks_cells_size = chunks_cells_size;
    this->number_of_chunks = number_of_chunks;
    this->chunks = new HeapChunk*[number_of_chunks];

    size_t block_info_list_size = sizeof(BlockInfo) * NUMBER_OF_BLOCKS;
    size_t heap_size = chunks_cells_size * BLOCKS_SIZE + block_info_list_size;
    size_t chunk_size = sizeof(HeapChunk) + heap_size;
    auto* entry = (uint8_t*) calloc(1, chunk_size * number_of_chunks);

    for (size_t i = 0; i < number_of_chunks; i++) {
        auto* chunk = (HeapChunk*) (entry + chunk_size * i);
        create_heap_chunk(chunk, chunks_cells_size);
        chunks[i] = chunk;
    }
}



#if SIZE_MAX == UINT32_MAX
    #define ADDRESS_SIZE_SHIFT 2
#elif SIZE_MAX == UINT64_MAX
    #define ADDRESS_SIZE_SHIFT 3
#endif

void* HeapAllocator::malloc(void* type_info, size_t fields_length, size_t* chunk_search_start_index) {
    size_t byte_size = sizeof(HeapObject) + (fields_length << ADDRESS_SIZE_SHIFT);

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
    } else if (byte_size <= BLOCK_SIZE12) {
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
        return nullptr;
    }

    size_t current_chunk_index = *chunk_search_start_index;

    this->lock.lock();
    HeapChunk** heap_chunks = this->chunks;
    size_t cs = this->number_of_chunks;
    this->lock.unlock();

    while (true) {
        size_t i = 0;
        while (true) {
            if (i == cs) {
                create_new_chunk(this->chunks_cells_size);
                this->lock.lock();
                heap_chunks = this->chunks;
                cs = this->number_of_chunks;
                this->lock.unlock();
                break;
            }

            HeapChunk* chunk = heap_chunks[current_chunk_index];

            void* address = chunk->malloc(type_info, index, block_size);
            if (address != nullptr) {
                *chunk_search_start_index = current_chunk_index;
                return address;
            }

            current_chunk_index++;
            if (current_chunk_index == cs) {
                current_chunk_index = 0;
            }

            i++;
        }
    }
}



void HeapAllocator::create_new_chunk(size_t cells_size) {
    auto* chunk = new HeapChunk(cells_size);

    this->lock.lock();
    size_t chunks_size_old = this->number_of_chunks;
    size_t chunks_size_new = chunks_size_old + 1;

    auto** heap_chunks_old = chunks;
    auto** heap_chunks_new = new HeapChunk*[chunks_size_new];
    for (size_t i = 0; i < chunks_size_old; i++) {
        heap_chunks_new[i] = heap_chunks_old[i];
    }
    heap_chunks_new[chunks_size_old] = chunk;

    this->chunks = heap_chunks_new;
    this->number_of_chunks = chunks_size_new;
    this->lock.unlock();
}


