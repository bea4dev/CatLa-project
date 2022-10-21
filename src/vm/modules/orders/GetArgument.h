#pragma once

#include <vm/modules/orders/Order.h>


class GetArgument : public Order {
private:
    size_t argument_index;

public:
    GetArgument(size_t result_register, size_t argument_index);
    void eval(void *vm_thread, void *module, uint64_t *registers, uint64_t *variables, uint64_t *arguments) override;
    void link(void *module, void *function) override;
};
