#include <vm/modules/orders/GetConst.h>
#include <vm/modules/Module.h>

void GetConstI64::eval(void* module, uint64_t* registers, uint64_t* variables) {
    auto mod = (Module*) module;
    registers[this->result_register] = *((uint64_t*) (mod->const_values[this->const_index].value_reference));
}

GetConstI64::GetConstI64(size_t result_register, size_t const_index) {
    this->result_register = result_register;
    this->const_index = const_index;
}
