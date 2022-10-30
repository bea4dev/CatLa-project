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


Module* parser::parse(const string& name, string* code) {
    size_t current_position = 0;
    const char* code_str = code->c_str();
    size_t code_length = code->size();

    ConstValue* const_values = nullptr;
    size_t const_values_size = 0;
    vector<string> import_module_names;
    vector<Type*> type_defines;
    vector<TypeInfo> using_types;
    vector<Function*> functions;

    string CONST = "$const";
    string IMPORT = "$import";
    string TYPE_DEF = "$typedef";
    string TYPE = "$type";
    string FUNCTION = "$function";
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
                type_defines = parse_type_define(code_str, code_length, &current_position, const_values);
            } else if (line == TYPE) {
                using_types = parse_type(code_str, code_length, &current_position);
            } else if (line == FUNCTION) {
                functions = parse_function(code_str, code_length, &current_position, const_values);
            }
        }

        auto* module = new Module(name, const_values, const_values_size, import_module_names, type_defines, using_types, functions);

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

        if (line.empty()) {
            current_position = cp;
            continue;
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

        if (line.empty()) {
            continue;
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


vector<Type*> parser::parse_type_define(const char *code, size_t code_length, size_t *position, ConstValue* const_values) {
    size_t current_position = *position;

    regex INFO_REG1(R"((\d+):\((.*)\):(\d+):(\d+))");
    regex INFO_REG2(R"((\d+):\((.*)\))");
    regex ARGS_SEPARATE{","};
    regex ARG_INFO1(R"((\d+):(\w+))");
    regex ARG_INFO2(R"((\d+):(\d+):(\d+))");
    string END = "$end";
    string CONST = "const";

    vector<Type*> types;

    string line;
    while (current_position != code_length) {
        line.clear();
        move_until_next_line(code, code_length, &current_position, &line);
        if (line == END) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        string name;
        size_t extends_import_index = 0;
        string extends_type_name;

        smatch results;
        if (regex_match(line, results, INFO_REG1)) {
            name = parser::get_const_value_as_string(const_values, stoull(results[1]));
            extends_import_index = stoull(results[3]);
            extends_type_name = parser::get_const_value_as_string(const_values, stoull(results[4]));
        } else if (regex_match(line, results, INFO_REG2)) {
            name = parser::get_const_value_as_string(const_values, stoull(results[1]));
        } else {
            throw ParseException();
        }

        auto fields_str = results[2].str();

        vector<FieldInfo> fields;
        auto fields_itr = sregex_token_iterator(fields_str.begin(), fields_str.end(), ARGS_SEPARATE, -1);
        auto end = sregex_token_iterator();
        while (fields_itr != end) {
            auto field_str = (*fields_itr++).str();

            if (regex_match(field_str, results, ARG_INFO1)) {
                auto field_name = results[1].str();
                auto* primitive_type = parser::parse_primitive_type(results[2].str().c_str());
                fields.push_back({field_name, primitive_type, {0, ""}});
            } else if (regex_match(field_str, results, ARG_INFO2)) {
                auto field_name = results[1].str();
                size_t import_index = stoull(results[2].str());
                auto type_name = parser::get_const_value_as_string(const_values, stoull(results[3].str()));
                fields.push_back({field_name, nullptr, {import_index, type_name}});
            } else {
                throw ParseException();
            }
        }

        types.push_back(new Type(nullptr, name, 0, fields, {extends_import_index, extends_type_name}));
    }

    *position = current_position;

    return types;
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

        if (line.empty()) {
            continue;
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
            auto type_name = results[3].str();

            type_map[index] = {import_index, type_name};
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

    regex INFO_REG(R"((\d+):(\d+):var:(\d+):reg:(\d+)\((.*)\)->(.+)\{)");
    regex ARGS_SEPARATE{","};
    regex ARGS_INFO(R"((\d+):(.+))");
    string END = "$end";

    unordered_map<size_t, Function*> function_map;

    size_t max_index = 0;
    string line;
    while (current_position != code_length) {
        line.clear();
        move_until_next_line(code, code_length, &current_position, &line);
        if (line == END) {
            break;
        }

        if (line.empty()) {
            continue;
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

            unordered_map<size_t, ArgumentInfo> arg_map;
            auto args_iterator = sregex_token_iterator(args_str.begin(), args_str.end(), ARGS_SEPARATE, -1);
            auto end = sregex_token_iterator();
            size_t max_arg_index = 0;
            while (args_iterator != end) {
                auto arg = (*args_iterator++).str();

                smatch arg_result;
                if (!regex_match(arg, arg_result, ARGS_INFO)) {
                    if (arg.empty()) {
                        break;
                    }
                    throw ParseException();
                } else {
                    size_t arg_index = stoull(arg_result[1].str());
                    auto type_info = parser::parse_argument_type(arg_result[2].str());
                    arg_map[arg_index] = type_info;

                    if (arg_index > max_arg_index) {
                        max_arg_index = arg_index;
                    }
                }
            }
            auto return_type_info = parser::parse_argument_type(results[6].str());

            if (!arg_map.empty()) {
                max_arg_index += 1;
            }
            vector<ArgumentInfo> argument_types(max_arg_index);
            for (size_t i = 0; i < max_arg_index; i++) {
                if (arg_map.find(i) != arg_map.end()) {
                    argument_types[i] = arg_map[i];
                } else {
                    throw ParseException();
                }
            }

            auto label_blocks = parser::parse_label_blocks(code, code_length, &current_position, const_values);

            function_map[index] = new Function(name, variables_size, registers_size, label_blocks, return_type_info, argument_types);
        }
    }

    *position = current_position;

    if (function_map.empty()) {
        return {};
    }

    size_t values_length = max_index + 1;
    vector<Function*> functions(values_length);
    for (size_t i = 0; i < values_length; i++) {
        if (function_map.find(i) != function_map.end()) {
            functions[i] = function_map[i];
        } else {
            throw ParseException();
        }
    }

    return functions;
}


vector<LabelBlock*> parser::parse_label_blocks(const char* code, size_t code_length, size_t* position, ConstValue* const_values) {
    size_t current_position = *position;

    regex INFO_REG(R"(label:(\w+))");
    string END = "}";

    vector<LabelBlock*> label_blocks;

    string line;
    while (current_position != code_length) {
        line.clear();
        move_until_next_line(code, code_length, &current_position, &line);
        if (line == END) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        smatch results;
        if (!regex_match(line, results, INFO_REG)) {
            throw ParseException();
        } else {
            auto label_name = results[1].str();
            auto orders = parser::parse_orders(code, code_length, &current_position, const_values);

            label_blocks.push_back(new LabelBlock(label_name, orders));
        }
    }

    *position = current_position;

    return label_blocks;
}


vector<Order*> parser::parse_orders(const char* code, size_t code_length, size_t* position, ConstValue* const_values) {
    size_t current_position = *position;

    regex ASSIGNMENT_REG(R"((.+)=(.+))");
    regex ARGS_SEPARATE{","};
    string END = "label:end";

    vector<Order*> orders;

    string line;
    while (current_position != code_length) {
        line.clear();
        move_until_next_line(code, code_length, &current_position, &line);
        if (line == END) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        smatch results;
        if (regex_match(line, results, ASSIGNMENT_REG)) {
            size_t assignment_register = parser::parse_register_or_variable_number(results[1].str());
            auto order_str = results[2].str();

            auto order_itr = sregex_token_iterator(order_str.begin(), order_str.end(), ARGS_SEPARATE, -1);
            auto end = sregex_token_iterator();

            string order_name;
            vector<string> args;
            size_t i = 0;
            while (order_itr != end) {
                auto arg_str = (*order_itr++).str();
                if (i == 0) {
                    order_name = arg_str;
                } else {
                    args.push_back(arg_str);
                }
                i++;
            }

            auto* order = parser::parse_order(assignment_register, order_name, args, const_values);
            orders.push_back(order);
        } else {
            auto order_itr = sregex_token_iterator(line.begin(), line.end(), ARGS_SEPARATE, -1);
            auto end = sregex_token_iterator();

            string order_name;
            vector<string> args;
            size_t i = 0;
            while (order_itr != end) {
                auto arg_str = (*order_itr++).str();
                if (i == 0) {
                    order_name = arg_str;
                } else {
                    args.push_back(arg_str);
                }
                i++;
            }

            auto* order = parser::parse_order(SIZE_MAX, order_name, args, const_values);
            orders.push_back(order);
        }
    }

    *position = current_position;

    return orders;
}


Order* parser::parse_order(size_t assignment_register, const string& order_name, vector<string> args, ConstValue* const_values) {
    if (assignment_register == SIZE_MAX) {
        if (order_name == "jump_to") {
            if (args.empty()) {
                throw ParseException();
            }
            return new JumpToLabel(args[0]);
        }

        if (order_name == "ret") {
            if (args.empty()) {
                throw ParseException();
            }
            size_t result_register = parser::parse_register_or_variable_number(args[0]);
            return new ReturnFunction(result_register);
        }

        if (order_name == "set_field") {
            if (args.size() < 5) {
                throw ParseException();
            }
            size_t parent_object_register_index = parser::parse_register_or_variable_number(args[0]);
            size_t field_object_register_index = parser::parse_register_or_variable_number(args[1]);
            bool borrow_lock = parser::parse_bool(args[2]);
            size_t using_type_index = parser::parse_using_type_index(args[3]);
            size_t field_name_const_value_index = parser::parse_const_value_index(args[4]);
            string field_name = parser::get_const_value_as_string(const_values, field_name_const_value_index);
            return new SetObjectFieldOwnership(parent_object_register_index, field_object_register_index, using_type_index, field_name, 0, borrow_lock);
        }
    } else {
        if (order_name == "const") {
            if (args.size() < 2) {
                throw ParseException();
            }
            auto* type = parser::parse_primitive_type(args[0].c_str());
            size_t const_index = stoull(args[1]);
            return new GetConstInteger(type, assignment_register, const_index);
        }

        if (order_name == "arg") {
            if (args.empty()) {
                throw ParseException();
            }
            size_t argument_index = stoull(args[0]);
            return new GetArgument(assignment_register, argument_index);
        }

        if (order_name == "iadd") {
            if (args.size() < 2) {
                throw ParseException();
            }
            auto* type = parser::parse_primitive_type(args[0].c_str());
            size_t left = parser::parse_register_or_variable_number(args[1]);
            size_t right = parser::parse_register_or_variable_number(args[2]);
            return new AddInteger(type, assignment_register, left, right);
        }

        if (order_name == "get_field") {

        }
    }

    throw ParseException();
}


size_t parser::parse_register_or_variable_number(const string& str) {
    regex REG(R"((reg|var)#(\d+))");
    smatch results;
    if (regex_match(str, results, REG)) {
        return stoull(results[2].str());
    } else if (str == "@void") {
        return SIZE_MAX;
    }
    throw ParseException();
}


size_t parser::parse_using_type_index(const string &str) {
    regex REG1(R"(type#(\d+))");
    regex REG2(R"((\d+))");
    smatch results;
    if (regex_match(str, results, REG1) || regex_match(str, results, REG2)) {
        return stoull(results[1].str());
    }
    throw ParseException();
}


size_t parser::parse_const_value_index(const string &str) {
    regex REG1(R"(const#(\d+))");
    regex REG2(R"((\d+))");
    smatch results;
    if (regex_match(str, results, REG1) || regex_match(str, results, REG2)) {
        return stoull(results[1].str());
    }
    throw ParseException();
}


bool parser::parse_bool(const string &str) {
    if (str == "true") {
        return true;
    } else if (str == "false") {
        return false;
    }
    throw ParseException();
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

ArgumentInfo parser::parse_argument_type(const string& type) {
    regex USER_TYPE_DEF_REG(R"(type#(\d+))");

    smatch results;
    if (regex_match(type, results, USER_TYPE_DEF_REG)) {
        size_t type_index = stoull(results[1].str());
        return {nullptr, type_index};
    } else {
        auto primitive_type = parser::parse_primitive_type(type.c_str());
        return {primitive_type, 0};
    }
}
