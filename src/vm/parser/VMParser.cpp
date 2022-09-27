#include <vm/parser/VMParser.h>

using namespace parser;

VMModule* parser::parse(string name, string* code) {
    size_t current_position = 0;
    //auto* const_values = parse_const(code, &current_position);
}


ConstValue* parser::parse_const(size_t code_length, const char* code, size_t* position) {
    size_t current_position = *position;

    while (current_position != code_length) {
        char current_char = code[current_position];
        char next_char = '\0';

        size_t next_position = current_position + 1;
        if (next_position < code_length) {
            next_char = code[next_position];
        }

        current_position++;
    }

    *position = current_position;
}