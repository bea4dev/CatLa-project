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

namespace cat_vm {

    extern atomic_size_t thread_ids;
    extern size_t reserved_threads;

    class VM_Thread {

    public:
        VM_Thread();
        ~VM_Thread();

        size_t thread_id;
    };


    class CatVM {

    private:
        mutex threads_manage_lock;
        vector<VM_Thread*> threads;

    public:
        CatVM();

        void run(CodeBlock *code_block);
    };
}
