#pragma once

#include <cstdint>
#include <vector>

using namespace std;

class Order {
protected:
    size_t result_register = SIZE_MAX;
public:
    virtual void eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables) = 0;
};

