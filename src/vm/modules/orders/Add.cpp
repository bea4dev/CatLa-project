#include <vm/modules/orders/Add.h>


AddI64::AddI64(size_t result_register, size_t left_register_index, size_t right_register_index) {
    this->result_register = result_register;
    this->left_register_index = left_register_index;
    this->right_register_index = right_register_index;
}

void AddI64::eval(void* vm_thread, void *module, uint64_t* registers, uint64_t* variables) {
    auto left = *((int64_t*) (registers + this->left_register_index));
    auto right = *((int64_t*) (registers + this->right_register_index));
    auto result = left + right;
    registers[this->result_register] = *((uint64_t*) &result);
}
