#pragma once

#include <vm/modules/orders/Order.h>

class GetObjectFieldAtomic : public Order {
private:
    size_t target_register;
    size_t using_type_index;
    string field_name;
    size_t field_index;

    GetObjectFieldAtomic(size_t target_register, size_t result_register, size_t using_type_index, const string& field_name);
    void eval(void *vm_thread, void *module, uint64_t *registers, uint64_t *variables, uint64_t *arguments) override;
    void link(void *module, void *function) override;
};