#include <vm/parser/VMParser.h>
#include <regex>
#include <unordered_map>
#include <CatLa.h>

using namespace std;
using namespace parser;

const char CATEGORY_DEFINE_END[] = ";\n\0";
const char HASH_MARK[] = "#\n\0";
const char LINE_END[] = "\n\0";


Module* parser::parse(string name, string* code) {
    size_t current_position = 0;
    const char* code_str = code->c_str();
    size_t code_length = code->size();

    ConstValue* const_values = nullptr;
    size_t const_values_size = 0;
    Module** imports = nullptr;
    size_t imports_size = 0;

    string CONST = "$const";
    string IMPORT = "$import";
    string line;
    while (current_position < code_length) {
        line.clear();
        move_until_next_line(code_str, code_length, &current_position, &line);

        if (line == CONST) {
            auto array = parse_const(code_str, code_length, &current_position);
            const_values = (ConstValue*) array.values;
            const_values_size = array.values_size;
            if (const_values == nullptr) {
                return nullptr;
            }
        } else if (line == IMPORT) {
            auto array = parse_import(code_str, code_length, &current_position, const_values);
            imports = (Module**) array.values;
            imports_size = array.values_size;
            if (imports == nullptr) {
                return nullptr;
            }
        }
    }

    auto* module = new Module(std::move(name), const_values, const_values_size, imports, imports_size);
    for (size_t i = 0; i < imports_size; i++) {
        if (imports[i] == nullptr) {
            imports[i] = module;
        }
    }

    return module;
}

Array parser::parse_const(const char* code, size_t code_length, size_t* position) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\d+):(\w))");
    string END = "$end";

    unordered_map<size_t, ConstValue> value_map;

    size_t max_index = 0;
    string line;
    while (current_position != code_length) {
        size_t cp = current_position;
        line.clear();
        move_until_next_line(code, code_length, &cp, &line);
        if (line == END) {
            current_position = cp;
            break;
        }

        line.clear();
        move_until(code, code_length, &current_position, HASH_MARK, 3, &line);

        smatch results;
        if (!regex_match(line, results, INFO_REG)) {
            return {nullptr, 0};
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            size_t byte_size = stoull(results[2].str());
            auto type_str = results[3].str();
            if (type_str.size() != 1) {
                return {nullptr, 0};
            }
            char type = type_str.c_str()[0];

            current_position++;
            if (current_position >= code_length) {
                return {nullptr, 0};
            }

            line.clear();
            for (size_t i = 0; i < byte_size; i++) {
                line.push_back(code[current_position]);
                current_position++;
            }

            void* value_reference;
            switch (type) {
                case 's': {
                    char* str = new char[byte_size];
                    const char* word_str = line.c_str();
                    for (size_t i = 0; i < byte_size; i++) {
                        str[i] = word_str[i];
                    }
                    value_reference = str;
                    break;
                }
                case 'i': {
                    auto* ir = new int64_t(stoll(line));
                    value_reference = ir;
                    break;
                }
                case 'f': {
                    auto* fr = new double(stod(line));
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

    if (value_map.empty()) {
        //TODO
    }

    size_t values_length = max_index + 1;
    auto* const_values = (ConstValue*) calloc(1, sizeof(ConstValue) * values_length);
    for (size_t i = 0; i < values_length; i++) {
        auto* const_value_reference = const_values + i;
        if (value_map.find(i) != value_map.end()) {
            auto* map_value_reference = &(value_map[i]);
            const_value_reference->byte_size = map_value_reference->byte_size;
            const_value_reference->type = map_value_reference->type;
            const_value_reference->value_reference = map_value_reference->value_reference;
        } else {
            return {nullptr, 0};
        }
    }

    return {const_values, values_length};
}


Array parser::parse_import(const char *code, size_t code_length, size_t *position, ConstValue* const_values) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\w+)#(\d+))");
    regex THIS_REG(R"((\d+):this)");
    string END = "$end";
    string CONST = "const";

    unordered_map<size_t, Module*> module_map;

    size_t max_index = 0;
    string line;
    while (current_position != code_length) {
        line.clear();
        move_until_next_line(code, code_length, &current_position, &line);
        if (line == END) {
            break;
        }

        smatch results;
        if (!regex_match(line, results, INFO_REG)) {
            if (regex_match(line, results, THIS_REG)) {
                size_t index = stoull(results[1].str());
                if (index > max_index) {
                    max_index = index;
                }

                module_map[index] = nullptr;
            } else {
                return {nullptr, 0};
            }
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            auto target = results[2].str();
            size_t target_index = stoull(results[3].str());

            Module* module = nullptr;
            if (target == CONST) {
                auto module_name = parser::get_const_value_as_string(const_values, target_index);
                module = virtual_machine->get_module(module_name);
            }

            module_map[index] = module;
        }
    }

    *position = current_position;

    if (module_map.empty()) {
        //TODO
    }

    size_t values_length = max_index + 1;
    auto** modules = new Module*[values_length];
    for (size_t i = 0; i < values_length; i++) {
        if (module_map.find(i) != module_map.end()) {
            modules[i] = module_map[i];
        } else {
            return {nullptr, 0};
        }
    }

    return {modules, values_length};
}


Array parser::parse_type(const char* code, size_t code_length, size_t* position, Module* modules) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\d+):(\d+))");
    string END = "$end";

    unordered_map<size_t, Type*> type_map;

    size_t max_index = 0;
    string line;
    while (current_position != code_length) {
        line.clear();
        move_until_next_line(code, code_length, &current_position, &line);
        if (line == END) {
            break;
        }

        smatch results;
        if (!regex_match(line, results, INFO_REG)) {
            return {nullptr, 0};
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            size_t import_index = stoull(results[2].str());
            size_t type_index = stoull(results[3].str());

            type_map[index] = nullptr;
        }
    }

    *position = current_position;

    if (type_map.empty()) {
        //TODO
    }

    size_t values_length = max_index + 1;
    auto** types = new Type*[values_length];
    for (size_t i = 0; i < values_length; i++) {
        if (type_map.find(i) != type_map.end()) {
            types[i] = type_map[i];
        } else {
            return {nullptr, 0};
        }
    }

    return {types, values_length};
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


void parser::move_until_next_line(const char *code, size_t code_length, size_t* position, string* line) {
    size_t current_position = *position;

    while (current_position != code_length) {
        char current_char = code[current_position];
        if (current_char == '\n') {
            current_position++;
            *position = current_position;
            return;
        }

        if (current_char != ' ') {
            line->push_back(current_char);
        }
        current_position++;
    }
    *position = current_position;
}


string parser::get_const_value_as_string(ConstValue* const_values, size_t index) {
    auto* const_value = const_values + index;
    char* value_reference = (char*) const_value->value_reference;
    size_t byte_size = const_value->byte_size;
    string str(value_reference, byte_size);
    return str;
}

int64_t parser::get_const_value_as_int64(ConstValue *const_values, size_t index) {
    auto* const_value = const_values + index;
    auto* value_reference = (int64_t*) const_value->value_reference;
    return *value_reference;
}

double parser::get_const_value_as_double(ConstValue *const_values, size_t index) {
    auto* const_value = const_values + index;
    auto* value_reference = (double*) const_value->value_reference;
    return *value_reference;
}