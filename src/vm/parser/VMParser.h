#pragma once

#include "vm/modules/VMModule.h"
#include <string>

using namespace std;

namespace parser {

    VMModule* parse(string name, string* code);

    ConstValue* parse_const(size_t code_length, const char* code, size_t* position);

}
