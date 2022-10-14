#include <vm/CatVM.h>
#include <stack>
#include <utility>
#include <vm/parser/VMParser.h>
#include <unordered_set>

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
    auto it = this->loaded_module_map.find(module_name);
    if (it != this->loaded_module_map.end()) {
        return (*it).second;
    }
    return nullptr;
}

string CatVM::get_pre_loaded_module(const string& module_name) {
    auto it = this->pre_loaded_module_map.find(module_name);
    if (it != this->pre_loaded_module_map.end()) {
        return (*it).second;
    }
    return "";
}


void CatVM::register_module(Module* module) {
    this->loaded_module_map[module->name] = module;
}

VMThread* CatVM::create_thread(size_t stack_size) {
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

void CatVM::pre_load_module(const string& name, string code) {
    this->pre_loaded_module_map[name] = std::move(code);
}

Module* CatVM::load_module(const string& name) {
    Module* target_module = nullptr;

    unordered_set<string> pre_load_modules;
    unordered_set<Module*> loaded;
    pre_load_modules.insert(name);

    string current_name;

    bool first = true;
    while (pre_load_modules.begin() != pre_load_modules.end()) {
        current_name = *pre_load_modules.begin();
        if (this->loaded_module_map.find(current_name) != this->loaded_module_map.end()) {
            pre_load_modules.erase(current_name);
            continue;
        }

        auto code = this->get_pre_loaded_module(current_name);
        if (code.empty()) {
            return nullptr; //TODO - error handling
        }

        auto* module = parser::parse(current_name, &code);
        if (first) {
            target_module = module;
            first = false;
        }

        loaded.insert(module);
        this->loaded_module_map[current_name] = module;
        pre_load_modules.erase(current_name);

        for (auto &it: module->import_module_names) {
            pre_load_modules.insert(it);
        }
    }

    for (auto& mod : loaded) {
        for (auto& it : mod->import_module_names) {
            auto* import_module = this->get_module(it);
            if (import_module == nullptr) {
                return nullptr; //TODO - error handling
            }

            mod->import_modules.push_back(import_module);
        }
    }

    for (auto& mod : loaded) {
        for (auto& type : mod->type_defines) {
            auto info = type->parent_info;
            size_t import_module_index = info.import_index;
            auto type_name = info.type_name;
            type->parent = mod->import_modules[import_module_index]->type_define_map[type_name];


        }
    }

    for (auto& mod : loaded) {
        for (auto& type_info : mod->using_type_infos) {
            size_t import_module_index = type_info.import_index;
            auto type_name = type_info.type_name;
            mod->using_types.push_back(mod->import_modules[import_module_index]->type_define_map[type_name]);
        }
    }

    return target_module;
}