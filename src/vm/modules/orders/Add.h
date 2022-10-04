#pragma once

#include <vm/modules/orders/Order.h>


class Add : public Order {
protected:
    size_t left_register_index = 0;
    size_t right_register_index = 0;

};


class AddI64 : public Add {
public:
    AddI64(size_t result_register, size_t left_register_index, size_t right_register_index);
    void eval(void *module, uint64_t* registers, uint64_t* variables) override;
};