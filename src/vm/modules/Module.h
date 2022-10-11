#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <vm/modules/Type.h>
#include <vm/modules/Function.h>

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
    Module** imports;
    size_t imports_size;

    Type** type_defines;
    size_t type_defines_size;

    vector<TypeInfo> using_type_infos;
    Type** using_types;
    size_t using_types_size;

    vector<Function*> functions;

public:
    explicit Module(string name, ConstValue* const_values, size_t const_values_size, vector<string> import_module_names, Type** type_defines, size_t type_defines_size, vector<TypeInfo> using_type_infos, vector<Function*> functions);
};
