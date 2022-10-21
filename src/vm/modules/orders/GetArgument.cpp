#include <vm/modules/orders/GetArgument.h>

GetArgument::GetArgument(size_t result_register, size_t argument_index) {
    this->result_register = result_register;
    this->argument_index = argument_index;
}

void GetArgument::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) {
    registers[this->result_register] = arguments[this->argument_index];
}

void GetArgument::link(void *module, void *function) {
    //None
}
