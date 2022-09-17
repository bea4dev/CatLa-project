#include "HeapRegion.h"

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


HeapRegion::HeapRegion(size_t cells_size) {
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
}


HeapRegion::~HeapRegion() {
    free((void*) this->entry_positions[0]);
}


#if SIZE_MAX == UINT32_MAX
    #define ADDRESS_SIZE_SHIFT 2
#elif SIZE_MAX == UINT64_MAX
    #define ADDRESS_SIZE_SHIFT 3
#endif

void* HeapRegion::alloc_for_class(CatLaClass* class_info, size_t refs_length, size_t vals_length) {
    size_t byte_size = sizeof(HeapObject) + (refs_length << ADDRESS_SIZE_SHIFT) + (vals_length << ADDRESS_SIZE_SHIFT);

    if (byte_size < BLOCK_SIZE8) {
        size_t index;
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
        size_t block_size = BLOCK_SIZE0 + (index << 3);
        uint8_t* block_entry = this->entry_positions[index];

        for (size_t address_index = 0; address_index < this->cells_size; address_index++) {
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
    } else if (byte_size < BLOCK_SIZE12) {
        size_t index;
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
        size_t block_size = BLOCK_SIZE9 + (index << 7);
        uint8_t* block_entry = this->entry_positions[index + 9];

        for (size_t address_index = 0; address_index < this->cells_size; address_index++) {
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
    } else {
        size_t index;
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
        size_t block_size = BLOCK_SIZE13 + (index << 10);
        uint8_t* block_entry = this->entry_positions[index + 13];

        for (size_t address_index = 0; address_index < this->cells_size; address_index++) {
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
    }

    return nullptr;
}