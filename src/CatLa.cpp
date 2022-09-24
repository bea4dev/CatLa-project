#include <iostream>
#include "CatLa.h"
#include <random>
#include "vm/Opcode.h"
#include <thread>
#include <vm/stack/stack.h>
#include <pthread.h>
#include <chrono>
#include <util/Benchmark.h>
#include <stack>
#include <util/Concurrent.h>
#include <heap/HeapAllocator.h>
#include <Python.h>

NyanVM* virtual_machine = nullptr;

void setup_virtual_machine() {
    virtual_machine = new NyanVM();
    reserved_threads = (size_t) thread::hardware_concurrency();
}


int random() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 10000);
    return dist(gen);
}

using namespace std;

uint64_t i = 0;
size_t top_of_stack;
size_t stack_size;

class StackOverflow {
public:
    StackOverflow() = default;
};

void detect_stack_overflow() {
    uint8_t a = 0;
    auto current_stack_address = (size_t) &a;
    if (stack_size - (top_of_stack - current_stack_address) <= 2000) {
        throw StackOverflow();
    }
}

void func() {
    detect_stack_overflow();
    i++;
    func();
}

void* start(void* arg) {
    uint8_t a = 0;
    top_of_stack = (size_t) &a;

    try {
        func();
    } catch (StackOverflow& exception) {
        printf("Stack overflow!! : %llu\n", i);
    }

    return nullptr;
}

struct TestStruct {
    uint8_t option;
    size_t length;
    concurrent::SpinLock sp_lock;
};

vector<size_t> heap_region_list;
size_t test(size_t address) {
    for (auto it = heap_region_list.begin(); it != heap_region_list.end(); ++it) {
        if (*it > address) {
            return address * 200 / *it;
        }
    }

    return 0;
}

GlobalHeap* global_heap;
size_t j = 0;
size_t c = 0;

HeapObject* create(int count) {
    if (count > 20) {
        c++;
        return (HeapObject*) global_heap->malloc(nullptr, 2, 0, &j);
        //return (HeapObject*) calloc(1, 40 + 16);
    }
    count++;
    c++;
    auto* parent = (size_t**) global_heap->malloc(nullptr, 2, 0, &j);
    //auto* parent = (size_t**) calloc(1, 40 + 16);
    auto* child1 = create(count);
    auto* child2 = create(count);
    if (parent != nullptr) {
        parent[5] = (size_t*) child1;
        parent[6] = (size_t*) child2;
    } else {
        printf("NULL! %d\n", c);
    }
    return (HeapObject*) parent;
}

using namespace benchmark;

int main()
{
    std::cout << "Hello World!\n";

    setup_virtual_machine();

    global_heap = new GlobalHeap(1024, 1);
    /*for (int t = 0; t < 600; t++) {
        global_heap->create_new_chunk(1024);
    }*/
    Py_Initialize();
    PyRun_SimpleString("print('hello, world!')");
    auto* module = PyModule_New("test");
    Py_Finalize();
    //return 0;

    Timing timing;
    printf("1\n");
    timing.start();
    create(0);

    /*for (int a = 0; a < 100; a++) {
        void* object = global_heap->malloc(nullptr, 2, 0, &j);
        if (object == nullptr) {
            printf("NULL!!!!\n");
        }
    }*/
    timing.end();
    printf("2\n");

    printf("%llu[ms]\n", timing.get_sum_time());


    std::cout << "Complete!\n";
}