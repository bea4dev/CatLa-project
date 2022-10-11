#include <vm/modules/orders/GetConst.h>
#include <vm/modules/Module.h>

GetConstInteger::GetConstInteger(PrimitiveType* type, size_t result_register, size_t const_index) {
    this->type = type;
    this->result_register = result_register;
    this->const_index = const_index;
}

void GetConstInteger::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables) {
    auto mod = (Module*) module;
    registers[this->result_register] = *((uint64_t*) (mod->const_values[this->const_index].value_reference));
}
