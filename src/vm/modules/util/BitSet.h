#pragma once

#include <cstdlib>
#include <cstdint>

inline uint8_t* create_bitset(size_t length) {
    return (uint8_t*) calloc(1, (length >> 3) + 1);
}

inline bool get_flag(const uint8_t* bytes, size_t index) {
    size_t byte_index = index >> 3;
    auto mask = (uint8_t) (1 << (index & 0b111));
    return bytes[byte_index] & mask;
}

inline void set_flag(uint8_t* bytes, size_t index, bool value) {
    size_t byte_index = index >> 3;
    bytes[byte_index] |= (uint8_t) ((uint8_t) value << (index & 0b111));
}