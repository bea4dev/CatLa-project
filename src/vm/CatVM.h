#pragma once
#include <vector>
#include <mutex>
#include <unordered_map>
#include <vm/modules/Module.h>
#include <vm/modules/Function.h>
#include <heap/HeapAllocator.h>
#include <stack>
#include <gc/GC.h>

namespace gc {
    class CycleCollector;
}
using namespace gc;

using namespace std;
using namespace modules;

class HeapAllocator;

namespace catla {

    extern atomic_size_t thread_ids;
    extern size_t reserved_threads;

    typedef struct {
        size_t thread_id;
        size_t stack_size;
        size_t top_of_stack_address;
        Function* current_function;
        LabelBlock* current_label_block;
        size_t current_order_index;
        bool return_function;
        HeapAllocator* heap_allocator;
        size_t allocator_search_start_index;
    } VMThread;


    class CatVM {

    private:
        mutex threads_manage_lock;
        vector<VMThread*> threads;
        unordered_map<string, string> pre_loaded_module_map;
        unordered_map<string, Module*> loaded_module_map;
        HeapAllocator* heap_allocator;
        vector<HeapChunk*> heap_chunk_list;
        SpinLock heap_chunk_list_lock;
        CycleCollector* cycle_collector;

    public:
        CatVM();

        uint64_t run_function(VMThread* vm_thread, Module* module, Function* function, uint64_t* arguments);
        Module* get_module(const string& module_name);
        string get_pre_loaded_module(const string& module_name);
        void register_module(Module* module);
        void pre_load_module(const string& name, string code);
        Module* load_module(const string& name);
        VMThread* create_thread(size_t stack_size);
        void add_heap_allocator_chunk(HeapChunk* chunk);
        vector<HeapChunk*> clone_heap_chunk_list();
        inline HeapAllocator* get_heap_allocator() {
            return heap_allocator;
        }
        inline CycleCollector* get_cycle_collector() {
            return cycle_collector;
        }
    };
}
