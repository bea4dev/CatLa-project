#pragma once

#include <string>

using namespace std;

typedef struct {
    void* values;
    size_t values_size;
} Array;

typedef struct {
    size_t import_index;
    size_t type_define_index;
} TypeInfo;
