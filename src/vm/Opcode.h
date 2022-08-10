#pragma once

#include <cstdint>

namespace cat_vm {
    enum opcode : uint8_t {
        set_info,
        push_const,
        set_reg,
        push_reg,
        heap_new,
        heap_delete,
        arc_hold,
        arc_drop,
        suspend,
        get_object_field,
        get_module_field,
        i8_add,
        i8_sub,
        i8_mul,
        i8_div,
        i8_rem,
        i8_l_shift,
        i8_r_shift,
        i8_ur_shift,
        i8_and,
        i8_or,
        i8_xor,
        i16_add,
        i16_sub,
        i16_mul,
        i16_div,
        i16_rem,
        i16_l_shift,
        i16_r_shift,
        i16_ur_shift,
        i16_and,
        i16_or,
        i16_xor,
        i32_add,
        i32_sub,
        i32_mul,
        i32_div,
        i32_rem,
        i32_l_shift,
        i32_r_shift,
        i32_ur_shift,
        i32_and,
        i32_or,
        i32_xor,
        i64_add,
        i64_sub,
        i64_mul,
        i64_div,
        i64_rem,
        i64_l_shift,
        i64_r_shift,
        i64_ur_shift,
        i64_and,
        i64_or,
        i64_xor,
        f32_add,
        f32_sub,
        f32_mul,
        f32_div,
        f32_rem,
        f64_add,
        f64_sub,
        f64_mul,
        f64_div,
        f64_rem,
    };
}
