#pragma once

#include <vm/modules/Module.h>
#include <vm/modules/Function.h>
#include <vm/PrimitiveType.h>
#include <string>
#include <exception>

using namespace std;
using namespace type;

namespace parser {


    class ParseException : std::exception {
    public:
        ParseException() = default;

        const char* what() const noexcept override {
            return "";
        }
    };


    Module* parse(const string& name, string* code);

    Array parse_const(const char* code, size_t code_length, size_t* position);

    vector<string> parse_import(const string& module_name, const char* code, size_t code_length, size_t* position, ConstValue* const_values);

    vector<Type*> parse_type_define(const char* code, size_t code_length, size_t* position, ConstValue* const_values);

    vector<TypeInfo> parse_type(const char* code, size_t code_length, size_t* position);

    vector<Function*> parse_function(const char* code, size_t code_length, size_t* position, ConstValue* const_values);

    vector<LabelBlock*> parse_label_blocks(const char* code, size_t code_length, size_t* position);

    vector<Order*> parse_orders(const char* code, size_t code_length, size_t* position);

    Order* parse_order(size_t assignment_register, const string& order_name, vector<string> args);

    size_t parse_register_or_variable_number(const string& str);

    void move_until(const char* code, size_t code_length, size_t* position, const char* chars, size_t chars_size, string* word);

    void move_until_next_line(const char* code, size_t code_length, size_t* position, string* line);

    string get_const_value_as_string(ConstValue* const_values, size_t index);

    int64_t get_const_value_as_int64(ConstValue* const_values, size_t index);

    double get_const_value_as_double(ConstValue* const_values, size_t index);

    PrimitiveType* parse_primitive_type(const char* type);

    ArgumentInfo parse_argument_type(const string& type);

}
