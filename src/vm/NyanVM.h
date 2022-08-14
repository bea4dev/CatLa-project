#pragma once
#include "vm.h"
#include <vector>
#include <mutex>

namespace heap {
    class HeapManager;
}

using namespace heap;
using namespace std;
using namespace modules;

namespace nyan {

    extern atomic_size_t thread_ids;
    extern size_t reserved_threads;

    class VMThread {

    public:
        VMThread();
        ~VMThread();

        size_t thread_id;
    };


    class NyanVM {

    private:
        mutex threads_manage_lock;
        vector<VMThread*> threads;

    public:
        NyanVM();

        static void run(VMThread* vm_thread, size_t thread_id, CodeBlock *code_block);
    };
}
