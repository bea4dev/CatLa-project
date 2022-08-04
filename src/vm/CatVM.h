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

    class VM_Thread {
    private:
        size_t thread_id;

    public:
        VM_Thread();
        ~VM_Thread();
    };


    class CatVM {

    private:
        mutex threads_manage_lock;
        vector<VM_Thread*> threads;

    public:
        CatVM();

        void run(const CodeBlock* code_block);
    };
}
