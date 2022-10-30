#include <vm/modules/orders/Add.h>


AddInteger::AddInteger(PrimitiveType* type, size_t result_register, size_t left_register_index, size_t right_register_index) {
    this->type = type;
    this->result_register = result_register;
    this->left_register_index = left_register_index;
    this->right_register_index = right_register_index;
}

void AddInteger::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) {
    if (type == type::i8) {
        auto left = *((int8_t*) (registers + this->left_register_index));
        auto right = *((int8_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::i16) {
        auto left = *((int16_t*) (registers + this->left_register_index));
        auto right = *((int16_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::i32) {
        auto left = *((int32_t*) (registers + this->left_register_index));
        auto right = *((int32_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::i64) {
        auto left = *((int64_t*) (registers + this->left_register_index));
        auto right = *((int64_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::u8) {
        auto left = *((uint8_t*) (registers + this->left_register_index));
        auto right = *((uint8_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::u16) {
        auto left = *((uint16_t*) (registers + this->left_register_index));
        auto right = *((uint16_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::u32) {
        auto left = *((uint32_t*) (registers + this->left_register_index));
        auto right = *((uint32_t*) (registers + this->right_register_index));
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    } else if (type == type::u64) {
        auto left = *(registers + this->left_register_index);
        auto right = *(registers + this->right_register_index);
        auto result = left + right;
        registers[this->result_register] = *((uint64_t*) &result);
    }
}

void AddInteger::link(void *module, void *function) {
    //None
}
