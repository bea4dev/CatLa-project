#include <vm/modules/Function.h>
#include <utility>

Function::Function(string name, size_t variables_size, size_t registers_size, vector<LabelBlock *> label_blocks, PrimitiveType* return_type, vector<PrimitiveType*> argument_types) {
    this->name = std::move(name);
    this->variables_size = variables_size;
    this->registers_size = registers_size;
    this->label_blocks = std::move(label_blocks);
    this->return_type = return_type;
    this->argument_types = std::move(argument_types);
}

LabelBlock::LabelBlock(string name, vector<Order*> orders) {
    this->name = std::move(name);
    this->orders = std::move(orders);
}
