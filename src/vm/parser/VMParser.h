#pragma once

#include "vm/modules/VMModule.h"
#include <string>

using namespace std;

namespace parser {

    VMModule* parse(string name, string* code);

    ConstValue* parse_const(const char* code, size_t code_length, size_t* position);

    void move_until(const char* code, size_t code_length, size_t* position, const char* chars, size_t chars_size, string* word);

}
