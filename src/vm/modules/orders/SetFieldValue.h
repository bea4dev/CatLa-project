#pragma once

#include <vm/modules/orders/Order.h>

class SetObjectFieldOwnership : public Order {
private:
    size_t target_register;
    size_t field_object_register_index;
    size_t using_type_index;
    string field_name;
    size_t field_index;
    bool borrow_lock;

    void eval(void *vm_thread, void *module, uint64_t *registers, uint64_t *variables, uint64_t *arguments) override;
    void link(void *module, void *function) override;

public:
    SetObjectFieldOwnership(size_t target_register, size_t field_object_register_index, size_t using_type_index, const string& field_name, size_t field_index, bool borrow_lock);
};