#include <vm/modules/Function.h>
#include <utility>

Function::Function(string name, size_t variables_size, size_t registers_size, vector<LabelBlock*> label_blocks, const ArgumentInfo& return_type_info, const vector<ArgumentInfo>& argument_type_infos) {
    this->name = std::move(name);
    this->variables_size = variables_size;
    this->registers_size = registers_size;
    this->label_blocks = std::move(label_blocks);
    this->return_type_info = return_type_info;
    this->return_type = nullptr;
    this->argument_type_infos = argument_type_infos;
}

LabelBlock::LabelBlock(string name, vector<Order*> orders) {
    this->name = std::move(name);
    this->orders = std::move(orders);
}
