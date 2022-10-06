#include <vm/CatVM.h>
#include "Opcode.h"
#include <stack>

using namespace catla;
using namespace heap;

namespace catla {
    atomic_size_t thread_ids;
    size_t reserved_threads = 1;
}

VMThread* catla::create_thread(size_t stack_size) {
    auto* thread = new VMThread;

    thread->thread_id = thread_ids.fetch_add(1);
    thread->stack_size = stack_size;
    uint8_t a = 0;
    thread->top_of_stack_address = (size_t) &a;

    return thread;
}


CatVM::CatVM() = default;

void CatVM::run(VMThread* vm_thread, Module* module, Function* function) {
    size_t registers_size = function->registers_size;
    size_t variables_size = function->variables_size;
    auto* registers = new uint64_t[registers_size];
    auto* variables = new uint64_t[variables_size];

    vm_thread->current_function = function;

    auto* current_label = function->label_blocks[0];
    vm_thread->current_label_block = current_label;
    vm_thread->current_order_index = 0;

    while (true) {
        auto* order = current_label->orders[vm_thread->current_order_index];
        order->eval(vm_thread, module, registers, variables);

        vm_thread->current_order_index++;
        if (vm_thread->current_order_index == vm_thread->current_label_block->orders.size()) {
            break;
        }
    }

    printf("Result : %lld\n", *((int64_t*) (registers + (registers_size - 1))));
}

Module* CatVM::get_module(const string& module_name) {
    if (this->loaded_module_map.find(module_name) != this->loaded_module_map.end()) {
        return this->loaded_module_map[module_name];
    }
    return nullptr;
}

void CatVM::register_module(Module* module) {
    this->loaded_module_map[module->name] = module;
}
