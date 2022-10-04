#pragma once
#include "vm.h"
#include <vector>
#include <mutex>
#include <unordered_map>
#include <vm/modules/Module.h>
#include <vm/modules/Function.h>

namespace heap {
    class HeapLifeManager;
}

using namespace heap;
using namespace std;
using namespace modules;

namespace nyan {

    extern atomic_size_t thread_ids;
    extern size_t reserved_threads;

    class VMThread {

    public:
        size_t thread_id;
        size_t stack_size;
        size_t top_of_stack_address;

    public:
        explicit VMThread(size_t stack_size);
        ~VMThread();
    };


    class NyanVM {

    private:
        mutex threads_manage_lock;
        vector<VMThread*> threads;
        unordered_map<string, Module*> loaded_module_map;

    public:
        NyanVM();

        static void run(VMThread* vm_thread, size_t thread_id, Module* module, Function* function);
        Module* get_module(const string& module_name);
        void register_module(Module* module);
    };
}
