#include <vm/modules/orders/New.h>
#include <vm/modules/Module.h>
#include <vm/CatVM.h>

using namespace catla;

NewClassObject::NewClassObject(size_t result_register, size_t using_type_index) {
    this->result_register = result_register;
    this->using_type_index = using_type_index;
    this->using_type = nullptr;
    this->field_length = 0;
}

void NewClassObject::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) {
    auto* thread = (VMThread*) vm_thread;
    auto* object = thread->heap_allocator->malloc(this->using_type, this->field_length, &thread->allocator_search_start_index);
    registers[this->result_register] = (uint64_t) object;
}

void NewClassObject::link(void* module, void* function) {
    auto* mod = (Module*) module;
    this->using_type = mod->using_types[this->using_type_index];
}


