#pragma once

#include <vm/modules/orders/Order.h>

class GetConstI64 : public Order {
private:
    size_t const_index;

public:
    GetConstI64(size_t result_register, size_t const_index);
    void eval(void *module, uint64_t* registers, uint64_t* variables) override;
};
