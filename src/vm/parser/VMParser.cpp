#include <vm/parser/VMParser.h>
#include <regex>
#include <unordered_map>

using namespace std;
using namespace parser;

const char CATEGORY_DEFINE_END[] = ";\n\0";
const char HASH_MARK[] = "#\n\0";
const char LINE_END[] = ";\n\0";


VMModule* parser::parse(string name, string* code) {
    size_t current_position = 0;
    const char* code_str = code->c_str();
    size_t code_length = code->size();

    ConstValue* const_values = nullptr;
    size_t const_values_size = 0;

    string CONST = "$const";
    string IMPORT = "$import";
    string word;
    while (current_position != code_length) {
        word.clear();
        move_until(code_str, code_length, &current_position, LINE_END, 3, &word);

        current_position += 2;
        if (current_position >= code_length) {
            break;
        }

        if (word == CONST) {
            auto array = parse_const(code_str, code_length, &current_position);
            const_values = (ConstValue*) array.values;
            const_values_size = array.values_size;
            if (const_values == nullptr) {
                return nullptr;
            }
        }
    }

    return new VMModule(const_values, const_values_size);
}

Array parser::parse_const(const char* code, size_t code_length, size_t* position) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\d+):(\w))");
    string END = "$end";

    unordered_map<size_t, ConstValue> value_map;

    size_t max_index = 0;
    string word;
    while (current_position != code_length) {
        size_t cp = current_position;
        word.clear();
        move_until(code, code_length, &cp, LINE_END, 3, &word);
        if (word == END) {
            break;
        }

        word.clear();
        move_until(code, code_length, &current_position, HASH_MARK, 3, &word);

        smatch results;
        if (!regex_match(word, results, INFO_REG)) {
            return {nullptr, 0};
        } else {
            size_t index = (size_t) stoi(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            size_t byte_size = (size_t) stoi(results[2].str());
            auto type_str = results[3].str();
            if (type_str.size() != 1) {
                return {nullptr, 0};
            }
            char type = type_str.c_str()[0];

            current_position++;
            if (current_position >= code_length) {
                return {nullptr, 0};
            }

            word.clear();
            for (size_t i = 0; i < byte_size; i++) {
                word.push_back(code[current_position]);
                current_position++;
            }

            void* value_reference;
            switch (type) {
                case 's': {
                    char* str = new char[byte_size];
                    const char* word_str = word.c_str();
                    for (size_t i = 0; i < byte_size; i++) {
                        str[i] = word_str[i];
                    }
                    value_reference = str;
                    break;
                }
                case 'i': {
                    auto* ir = new int64_t(stoll(word));
                    value_reference = ir;
                    break;
                }
                case 'f': {
                    auto* fr = new double(stod(word));
                    value_reference = fr;
                    break;
                }
                default: {
                    return {nullptr, 0};
                }
            }

            ConstValue const_value = {byte_size, type, value_reference};
            value_map[index] = const_value;
        }

        current_position++;
    }

    *position = current_position;

    size_t values_length = max_index + 1;
    auto* const_values = (ConstValue*) calloc(1, sizeof(ConstValue) * values_length);
    for (size_t i = 0; i < values_length; i++) {
        auto* const_value_reference = const_values + i;
        if (value_map.find(i) != value_map.end()) {
            const_value_reference->byte_size = value_map[i].byte_size;
            const_value_reference->type = value_map[i].type;
            const_value_reference->value_reference = value_map[i].value_reference;
        }
    }

    return {const_values, values_length};
}


Array parser::parse_import(const char *code, size_t code_length, size_t *position) {
    size_t current_position = *position;
    return {nullptr, 0};
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