#pragma once

#include <cstdint>

namespace catla {

    enum primitive_type : uint8_t {
        i8,
        i16,
        i32,
        i64,
        u8,
        u16,
        u32,
        u64,
        ref,
    };

}