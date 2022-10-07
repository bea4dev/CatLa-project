#include <vm/parser/VMParser.h>
#include <regex>
#include <unordered_map>
#include <CatLa.h>
#include <vm/modules/Type.h>
#include <vm/parser/Structs.h>
#include <vm/PrimitiveType.h>

using namespace std;
using namespace parser;
using namespace modules;
using namespace type;

const char HASH_MARK[] = "#\n\0";


Module* parser::parse(string name, string* code) {
    size_t current_position = 0;
    const char* code_str = code->c_str();
    size_t code_length = code->size();

    ConstValue* const_values = nullptr;
    size_t const_values_size = 0;
    vector<string> import_module_names;
    Type** type_defines = nullptr;
    size_t type_defines_size = 0;
    vector<TypeInfo> using_types;

    string CONST = "$const";
    string IMPORT = "$import";
    string TYPE_DEF = "$typedef";
    string TYPE = "$type";
    string line;

    try {
        while (current_position < code_length) {
            line.clear();
            move_until_next_line(code_str, code_length, &current_position, &line);

            if (line == CONST) {
                auto array = parse_const(code_str, code_length, &current_position);
                const_values = (ConstValue*) array.values;
                const_values_size = array.values_size;
            } else if (line == IMPORT) {
                import_module_names = parse_import(name, code_str, code_length, &current_position, const_values);
            } else if (line == TYPE_DEF) {
                auto array = parse_type_define(code_str, code_length, &current_position, const_values);
                type_defines = (Type**) array.values;
                type_defines_size = array.values_size;
            } else if (line == TYPE) {
                using_types = parse_type(code_str, code_length, &current_position);
            }
        }

        auto* module = new Module(name, const_values, const_values_size, std::move(import_module_names), type_defines, type_defines_size, std::move(using_types));

        return module;
    } catch (const ParseException& e) {
        printf("Parse exception!\n");
    }

    return nullptr;
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
            throw ParseException();
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            size_t byte_size = stoull(results[2].str());
            auto type_str = results[3].str();
            if (type_str.size() != 1) {
                throw ParseException();
            }
            char type = type_str.c_str()[0];

            current_position++;
            if (current_position >= code_length) {
                throw ParseException();
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
                    throw ParseException();
                }
            }

            ConstValue const_value = {byte_size, type, value_reference};
            value_map[index] = const_value;
        }

        current_position++;
    }

    *position = current_position;

    if (value_map.empty()) {
        return {nullptr, 0};
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
            throw ParseException();
        }
    }

    return {const_values, values_length};
}


vector<string> parser::parse_import(const string& module_name, const char *code, size_t code_length, size_t *position, ConstValue* const_values) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\w+)#(\d+))");
    regex THIS_REG(R"((\d+):this)");
    string END = "$end";
    string CONST = "const";

    unordered_map<size_t, string> module_map;

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

                module_map[index] = module_name;
            } else {
                throw ParseException();
            }
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            auto target = results[2].str();
            size_t target_index = stoull(results[3].str());

            string module;
            if (target == CONST) {
                module = parser::get_const_value_as_string(const_values, target_index);
            }

            module_map[index] = module;
        }
    }

    *position = current_position;

    if (module_map.empty()) {
        return {};
    }

    size_t values_length = max_index + 1;
    vector<string> module_names(values_length);
    for (size_t i = 0; i < values_length; i++) {
        if (module_map.find(i) != module_map.end()) {
            module_names[i] = module_map[i];
        } else {
            throw ParseException();
        }
    }

    return module_names;
}


Array parser::parse_type_define(const char *code, size_t code_length, size_t *position, ConstValue* const_values) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\d+):(\d+):(\d+):\((.*)\))");
    regex EXTENDS_SEPARATE{","};
    regex EXTENDS_INFO(R"((\d+):(\d+))");
    string END = "$end";
    string CONST = "const";

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
            throw ParseException();
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            auto name = parser::get_const_value_as_string(const_values, stoull(results[2].str()));
            size_t refs_length = stoull(results[3].str());
            size_t vals_length = stoull(results[4].str());


            vector<TypeInfo> parent_infos;

            auto extends = results[5].str();
            auto extends_iterator = sregex_token_iterator(extends.begin(), extends.end(), EXTENDS_SEPARATE, -1);
            auto end = sregex_token_iterator();
            while (extends_iterator != end) {
                auto type_string = (*extends_iterator++).str();

                smatch type_result;
                if (!regex_match(type_string, type_result, EXTENDS_INFO)) {
                    throw ParseException();
                } else {
                    size_t import_index = stoull(type_result[1].str());
                    size_t type_define_index = stoull(type_result[2].str());

                    parent_infos.push_back({import_index, type_define_index});
                }
            }

            auto* type = new Type(name, 0, refs_length, vals_length, parent_infos);
            type_map[index] = type;
        }
    }

    *position = current_position;

    if (type_map.empty()) {
        return {nullptr, 0};
    }

    size_t values_length = max_index + 1;
    auto** types = new Type*[values_length];
    for (size_t i = 0; i < values_length; i++) {
        if (type_map.find(i) != type_map.end()) {
            types[i] = type_map[i];
        } else {
            throw ParseException();
        }
    }

    return {types, values_length};
}


vector<TypeInfo> parser::parse_type(const char* code, size_t code_length, size_t* position) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\d+):(\d+))");
    string END = "$end";

    unordered_map<size_t, TypeInfo> type_map;

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
            throw ParseException();
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            size_t import_index = stoull(results[2].str());
            size_t type_define_index = stoull(results[3].str());

            type_map[index] = {import_index, type_define_index};
        }
    }

    *position = current_position;

    if (type_map.empty()) {
        return {};
    }

    size_t values_length = max_index + 1;
    vector<TypeInfo> type_infos(values_length);
    for (size_t i = 0; i < values_length; i++) {
        if (type_map.find(i) != type_map.end()) {
            type_infos[i] = type_map[i];
        } else {
            throw ParseException();
        }
    }

    return type_infos;
}


vector<Function*> parser::parse_function(const char* code, size_t code_length, size_t* position, ConstValue* const_values) {
    size_t current_position = *position;

    regex INFO_REG(R"((\d+):(\d+):var:(\d+):reg:(\d+)\((\w*)\)->(\w+){)");
    regex ARGS_SEPARATE{","};
    regex ARGS_INFO(R"((\d+):(\w+))");
    string END = "$end";

    unordered_map<size_t, Function*> function_map;
    unordered_map<size_t, PrimitiveType*> arg_map;

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
            throw ParseException();
        } else {
            size_t index = stoull(results[1].str());
            if (index > max_index) {
                max_index = index;
            }

            string name = parser::get_const_value_as_string(const_values, stoull(results[2].str()));
            size_t variables_size = stoull(results[3].str());
            size_t registers_size = stoull(results[4].str());

            string args_str = results[5].str();

            auto args_iterator = sregex_token_iterator(args_str.begin(), args_str.end(), ARGS_SEPARATE, -1);
            auto end = sregex_token_iterator();
            while (args_iterator != end) {
                auto arg = (*args_iterator++).str();

                smatch arg_result;
                if (!regex_match(arg, arg_result, ARGS_INFO)) {
                    throw ParseException();
                } else {
                    size_t arg_index = stoull(arg_result[1].str());
                    auto* type = parser::parse_primitive_type(arg_result[2].str().c_str());
                    arg_map[arg_index] = type;
                }
            }

        }
    }

    *position = current_position;
/*
    if (type_map.empty()) {
        return {};
    }

    size_t values_length = max_index + 1;
    vector<TypeInfo> type_infos(values_length);
    for (size_t i = 0; i < values_length; i++) {
        if (type_map.find(i) != type_map.end()) {
            type_infos[i] = type_map[i];
        } else {
            throw ParseException();
        }
    }

    return type_infos;*/return {};
}


vector<LabelBlock*> parser::parse_label_block(const char* code, size_t code_length, size_t* position) {

}


vector<Order *> parser::parse_order(const char *code, size_t code_length, size_t *position) {

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

PrimitiveType* parser::parse_primitive_type(const char* type) {
    for (auto &it : *type::all_types) {
        if (strcmp(it->name, type) == 0) {
            return it;
        }
    }
    throw ParseException();
}