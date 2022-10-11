#pragma once

#include <vm/modules/orders/Order.h>


class Add : public Order {
protected:
    size_t left_register_index = 0;
    size_t right_register_index = 0;

};


class AddInteger : public Add {
public:
    AddInteger(PrimitiveType* type, size_t result_register, size_t left_register_index, size_t right_register_index);
    void eval(void* vm_thread, void *module, uint64_t* registers, uint64_t* variables) override;
};