#pragma once

#include "vm/modules/Module.h"
#include <string>

using namespace std;

typedef struct {
    void* values;
    size_t values_size;
} Array;

namespace parser {

    Module* parse(string name, string* code);

    Array parse_const(const char* code, size_t code_length, size_t* position);

    Array parse_import(const char* code, size_t code_length, size_t* position, ConstValue* const_values);

    Array parse_type(const char* code, size_t code_length, size_t* position, Module* modules);

    void move_until(const char* code, size_t code_length, size_t* position, const char* chars, size_t chars_size, string* word);

    void move_until_next_line(const char* code, size_t code_length, size_t* position, string* line);

    string get_const_value_as_string(ConstValue* const_values, size_t index);

    int64_t get_const_value_as_int64(ConstValue* const_values, size_t index);

    double get_const_value_as_double(ConstValue* const_values, size_t index);

}
