#include <vm/PrimitiveType.h>
#include <vm/modules/Type.h>

using namespace type;
using namespace modules;

namespace type {
    vector<PrimitiveType *> *all_types;

    PrimitiveType* type_void;
    PrimitiveType* i8;
    PrimitiveType* i16;
    PrimitiveType* i32;
    PrimitiveType* i64;
    PrimitiveType* u8;
    PrimitiveType* u16;
    PrimitiveType* u32;
    PrimitiveType* u64;

}

type::PrimitiveType* type::create(uint8_t id, const char* name) {
    auto* primitive_type = new PrimitiveType{id, name, nullptr};
    auto* type = new Type(primitive_type, "", 0, {}, {0, ""});
    primitive_type->type = type;

    all_types->push_back(primitive_type);
    return primitive_type;
}

void type::setup_primitive_types() {
    all_types = new vector<PrimitiveType*>;

    type_void = create(0, "void");
    i8 = create(1, "i8");
    i16 = create(2, "i16");
    i32 = create(3, "i32");
    i64 = create(4, "i64");
    u8 = create(5, "u8");
    u16 = create(6, "u16");
    u32 = create(7, "u32");
    u64 = create(8, "u64");
}