#include <vm/PrimitiveType.h>

using namespace type;

namespace type {
    vector<PrimitiveType *> *all_types;

    PrimitiveType *i8;
    PrimitiveType *i16;
    PrimitiveType *i32;
    PrimitiveType *i64;
    PrimitiveType *u8;
    PrimitiveType *u16;
    PrimitiveType *u32;
    PrimitiveType *u64;
    PrimitiveType *ref;
}

type::PrimitiveType* type::create(uint8_t id, const char* name) {
    auto* type = new PrimitiveType{id, name};
    all_types->push_back(type);
    return type;
}

void type::setup_primitive_types() {
    all_types = new vector<PrimitiveType*>;

    i8 = create(0, "i8");
    i16 = create(1, "i16");
    i32 = create(2, "i32");
    i64 = create(3, "i64");
    u8 = create(4, "u8");
    u16 = create(5, "u16");
    u32 = create(6, "u32");
    u64 = create(7, "u64");
    ref = create(8, "ref");
}