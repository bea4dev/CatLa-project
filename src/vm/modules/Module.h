#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <vm/modules/Type.h>
#include <vm/modules/Function.h>
#include <unordered_map>

using namespace std;
using namespace modules;

typedef struct {
    size_t byte_size;
    char type;
    void* value_reference;
} ConstValue;

class Module {
public:
    string name;
    ConstValue* const_values;
    size_t const_values_size;

    vector<string> import_module_names;
    vector<Module*> import_modules;

    unordered_map<string, Type*> type_define_map;
    vector<Type*> type_defines;

    vector<TypeInfo> using_type_infos;
    vector<Type*> using_types;

    vector<Function*> functions;

    void* module_fields = nullptr;

public:
    Module(string name, ConstValue *const_values, size_t const_values_size, const vector<string> &import_module_names,
           const vector<Type *> &type_defines, const vector<TypeInfo> &using_type_infos,
           const vector<Function *> &functions);
};
