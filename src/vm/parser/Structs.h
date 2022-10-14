#pragma once

#include <string>
#include <vm/PrimitiveType.h>

using namespace std;
using namespace type;

typedef struct {
    void* values;
    size_t values_size;
} Array;

typedef struct {
    size_t import_index;
    string type_name;
} TypeInfo;

typedef struct {
    string field_name;
    PrimitiveType* primitive_type;
    TypeInfo user_def_type;
} FieldInfo;
