#include <vm/modules/orders/ReturnFunction.h>
#include <vm/CatVM.h>

using namespace catla;

ReturnFunction::ReturnFunction(size_t result_register) {
    this->result_register = result_register;
}

void ReturnFunction::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t *variables, uint64_t *arguments) {
    auto* thread = (VMThread*) vm_thread;
    thread->return_function = true;
    auto* function = thread->current_function;
    registers[function->registers_size] = registers[this->result_register];
}
