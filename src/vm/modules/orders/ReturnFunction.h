#pragma once

#include <vm/modules/orders/Order.h>


class ReturnFunction : public Order {

public:
    ReturnFunction(size_t result_register);
    void eval(void *vm_thread, void *module, uint64_t *registers, uint64_t *variables, uint64_t *arguments) override;
};