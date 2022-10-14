#pragma once

#include <string>
#include <vector>

using namespace std;

namespace type {

    typedef struct {
        uint8_t id;
        const char* name;
        void* type;
    } PrimitiveType;


    extern vector<PrimitiveType*>* all_types;

    extern PrimitiveType* type_void;
    extern PrimitiveType* i8;
    extern PrimitiveType* i16;
    extern PrimitiveType* i32;
    extern PrimitiveType* i64;
    extern PrimitiveType* u8;
    extern PrimitiveType* u16;
    extern PrimitiveType* u32;
    extern PrimitiveType* u64;

    extern PrimitiveType* create(uint8_t id, const char* name);

    extern void setup_primitive_types();

}