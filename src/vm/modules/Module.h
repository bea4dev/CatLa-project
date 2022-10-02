#pragma once

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

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
    Module** imports;
    size_t imports_size;

public:
    explicit Module(string name, ConstValue* const_values, size_t const_values_size, Module** imports, size_t imports_size);
};
