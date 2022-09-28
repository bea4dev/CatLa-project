#include <vm/parser/VMParser.h>

using namespace parser;

const char CATEGORY_DEFINE_END[] = "; \n\0";
const char HASH_MARK[] = "#\n\0";
const char LINE_END[] = "\n\0";


VMModule* parser::parse(string name, string* code) {
    size_t current_position = 0;
    const char* code_str = code->c_str();
    size_t code_length = code->size();

    ConstValue* const_values = nullptr;

    string CONST = "$const";
    string word;
    while (current_position != code_length) {
        word.clear();
        move_until(code_str, code_length, &current_position, CATEGORY_DEFINE_END, 4, &word);
        current_position++;

        if (word == CONST) {
            const_values = parse_const(code_str, code_length, &current_position);
        }
    }

    return new VMModule(const_values);
}

ConstValue* parser::parse_const(const char* code, size_t code_length, size_t* position) {
    size_t current_position = *position;

    string word;
    while (current_position != code_length) {
        word.clear();
        move_until(code, code_length, &current_position, LINE_END, 2, &word);



        current_position++;
    }

    *position = current_position;
}


void parser::move_until(const char* code, size_t code_length, size_t* position, const char* chars, size_t chars_size, string* word) {
    size_t current_position = *position;

    while (current_position != code_length) {
        char current_char = code[current_position];
        for (size_t i = 0; i < chars_size; i++) {
            if (current_char == chars[i]) {
                *position = current_position;
                return;
            }
        }

        if (current_char != ' ') {
            word->push_back(current_char);
        }
        current_position++;
    }
}