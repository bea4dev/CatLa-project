#pragma once

#include "vm/modules/VMModule.h"
#include <string>

using namespace std;

typedef struct {
    void* values;
    size_t values_size;
} Array;

namespace parser {

    VMModule* parse(string name, string* code);

    Array parse_const(const char* code, size_t code_length, size_t* position);

    Array parse_import(const char* code, size_t code_length, size_t* position);

    void move_until(const char* code, size_t code_length, size_t* position, const char* chars, size_t chars_size, string* word);

}
