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

class VMModule {
public:
    ConstValue* const_values;
    size_t const_values_size;

public:
    explicit VMModule(ConstValue* const_values, size_t const_values_size);
};
