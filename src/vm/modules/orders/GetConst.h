#pragma once

#include <vm/modules/orders/Order.h>

class GetConstInteger : public Order {
private:
    size_t const_index;

public:
    GetConstInteger(PrimitiveType* type, size_t result_register, size_t const_index);
    void eval(void* vm_thread, void *module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) override;
    void link(void *module, void *function) override;
};
