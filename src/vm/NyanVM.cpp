#include <vm/NyanVM.h>
#include "Opcode.h"
#include <stack>

using namespace nyan;
using namespace heap;

namespace nyan {
    atomic_size_t thread_ids;
    size_t reserved_threads = 1;
}

VMThread::VMThread(size_t stack_size) {
    this->thread_id = thread_ids.fetch_add(1);
    this->stack_size = stack_size;
    uint8_t a = 0;
    this->top_of_stack_address = (size_t) &a;
}

VMThread::~VMThread() = default;


NyanVM::NyanVM() = default;

void NyanVM::run(VMThread* vm_thread, size_t thread_id, Module* module, Function* function) {

}

Module* NyanVM::get_module(const string& module_name) {
    if (this->loaded_module_map.find(module_name) != this->loaded_module_map.end()) {
        return this->loaded_module_map[module_name];
    }
    return nullptr;
}

void NyanVM::register_module(Module* module) {
    this->loaded_module_map[module->name] = module;
}
