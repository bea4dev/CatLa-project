#pragma once
#include <vm/modules/orders/Order.h>
#include <string>

using namespace std;


class JumpToLabel : public Order {
private:
    string label_name;
    void* label;

public:
    explicit JumpToLabel(string label_name);
    void eval(void* vm_thread, void *module, uint64_t *registers, uint64_t *variables, uint64_t* arguments) override;
};