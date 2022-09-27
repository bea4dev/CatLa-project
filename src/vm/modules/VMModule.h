#pragma once

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

typedef struct {
    size_t length;
    void* entry_position;
} ConstValue;

class VMModule {
public:
    ConstValue* const_values;

public:
    explicit VMModule(ConstValue* const_values);
};
