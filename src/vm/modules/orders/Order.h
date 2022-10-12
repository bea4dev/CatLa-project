#pragma once

#include <cstdint>
#include <vector>
#include <vm/PrimitiveType.h>

using namespace std;
using namespace type;

class Order {
protected:
    size_t result_register = SIZE_MAX;
    PrimitiveType* type = nullptr;

public:
    virtual void eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) = 0;
};

