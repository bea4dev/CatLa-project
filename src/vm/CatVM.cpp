#include <vm/CatVM.h>
#include <stack>

using namespace catla;
using namespace heap;

namespace catla {
    atomic_size_t thread_ids;
    size_t reserved_threads = 1;
}

CatVM::CatVM() = default;

uint64_t CatVM::run_function(VMThread* vm_thread, Module* module, Function* function, uint64_t* arguments) {
    size_t registers_size = function->registers_size;
    size_t variables_size = function->variables_size;
    auto* registers = new uint64_t[registers_size + 1];
    auto* variables = new uint64_t[variables_size];

    vm_thread->current_function = function;

    auto* current_label = function->label_blocks[0];
    vm_thread->current_label_block = current_label;
    vm_thread->current_order_index = 0;

    while (true) {
        auto* order = vm_thread->current_label_block->orders[vm_thread->current_order_index];
        order->eval(vm_thread, module, registers, variables, arguments);

        vm_thread->current_order_index++;
        if (vm_thread->current_order_index == vm_thread->current_label_block->orders.size() || vm_thread->return_function) {
            vm_thread->return_function = false;
            break;
        }
    }


    for (size_t i = 0; i <= registers_size; i++) {
        printf("register : %llu | value : %lld\n", i, registers[i]);
    }

    uint64_t result = registers[registers_size];
    delete[] variables;
    delete[] registers;

    return result;
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

VMThread *CatVM::create_thread(size_t stack_size) {
    auto* thread = new VMThread;

    thread->thread_id = thread_ids.fetch_add(1);
    thread->stack_size = stack_size;
    uint8_t a = 0;
    thread->top_of_stack_address = (size_t) &a;
    thread->return_function = false;

    this->threads_manage_lock.lock();
    this->threads.push_back(thread);
    this->threads_manage_lock.unlock();

    return thread;
}
