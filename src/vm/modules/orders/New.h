#pragma once

#include <vm/modules/orders/Order.h>
#include <vm/modules/Type.h>

using namespace modules;

class NewClassObject : public Order {
private:
    size_t using_type_index;
    Type* using_type;
    size_t field_length;

public:
    NewClassObject(size_t result_register, size_t using_type_index);
    void eval(void *vm_thread, void *module, uint64_t *registers, uint64_t *variables, uint64_t *arguments) override;
    void link(void *module, void *function) override;
};

