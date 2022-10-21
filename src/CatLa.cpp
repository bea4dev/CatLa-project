﻿#include <iostream>
#include "CatLa.h"
#include <random>
#include <vm/PrimitiveType.h>
#include <thread>
#include <vm/stack/stack.h>
#include <pthread.h>
#include <chrono>
#include <util/Benchmark.h>
#include <stack>
#include <util/Concurrent.h>
#include <heap/HeapAllocator.h>
#include <Python.h>
#include <string>
#include <regex>
#include <util/StringUtil.h>
#include <vm/parser/VMParser.h>
#include <vm/PrimitiveType.h>

CatVM* virtual_machine = nullptr;

void setup_virtual_machine() {
    type::setup_primitive_types();
    virtual_machine = new CatVM();
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

HeapAllocator* global_heap;
size_t j = 0;
size_t c = 0;

HeapObject* create(int count) {
    if (count > 20) {
        c++;
        return (HeapObject*) global_heap->malloc(nullptr, 2, &j);
        //return (HeapObject*) calloc(1, 40 + 16);
        //return (HeapObject*) malloc(40 + 16);
    }
    count++;
    c++;
    auto* parent = (size_t**) global_heap->malloc(nullptr, 2, &j);
    //auto* parent = (size_t**) calloc(1, 40 + 16);
    //auto* parent = (size_t**) malloc(40 + 16);
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

size_t a = 0;
int b = 0;

void* func(void* args) {
    for (int s = 0; s < 10000; s++) {
        auto* flag = (atomic_flag*) &a;
        while (flag->test_and_set(std::memory_order_acquire)) {
            //wait
        }
        b++;
        flag->clear(std::memory_order_release);
    }
    return nullptr;
}

bool f = true;
size_t n = 0;
atomic_size_t m(0);
atomic_flag flag;

int main()
{
    std::cout << "Hello World!\n";

    printf("%llu\n", sizeof(HeapObject));

    pthread_t pthread1;
    pthread_t pthread2;
    pthread_t pthread3;
    pthread_t pthread4;
    pthread_attr_t thread_attribute;
    pthread_attr_init(&thread_attribute);
    pthread_create(&pthread1, &thread_attribute, func, nullptr);
    pthread_create(&pthread2, &thread_attribute, func, nullptr);
    pthread_create(&pthread3, &thread_attribute, func, nullptr);
    pthread_create(&pthread4, &thread_attribute, func, nullptr);
    pthread_join(pthread1, nullptr);
    pthread_join(pthread2, nullptr);
    pthread_join(pthread3, nullptr);
    pthread_join(pthread4, nullptr);
    printf("%d\n", b);

    Timing timing1;
    Timing timing2;

    timing1.start();
    for (int s = 0; s < 4194303; s++) {
        if (f) {
            n++;
        }
    }
    //atomic_thread_fence(std::memory_order_release);
    timing1.end();



    timing2.start();
    for (int s = 0; s < 4194303; s++) {
        if (f) {
            while (flag.test_and_set(memory_order_acquire)) {
                //lock
            }
            m++;
            flag.clear(memory_order_release);
        }
    }
    timing2.end();

    printf("%llu[ms]\n", timing1.get_sum_time());
    printf("%llu[ms]\n", timing2.get_sum_time());

    setup_virtual_machine();

    global_heap = new HeapAllocator(1024, 1);
    /*for (int t = 0; t < 600; t++) {
        global_heap->create_new_chunk(1024);
    }*/


    Py_Initialize();
    PyRun_SimpleString("print('hello, world!')");
    Py_Finalize();
    //return 0;

    Timing timing;
    timing.start();
    create(0);

    /*for (int a = 0; a < 100; a++) {
        void* object = global_heap->malloc(nullptr, 2, 0, &j);
        if (object == nullptr) {
            printf("NULL!!!!\n");
        }
    }*/
    timing.end();

    printf("%llu[ms]\n", timing.get_sum_time());

    string vm_code = u8"$const\n"
                     "  0:4:s#nyan\n"
                     "  1:2:i#20\n"
                     "  2:3:i#100\n"
                     "  3:4:i#1000\n"
                     "  4:9:s#TestClass\n"
                     "  5:10:s#TestClass2\n"
                     "  6:1:s#a\n"
                     "  7:1:s#b\n"
                     "  8:1:s#c\n"
                     "$end\n"
                     "\n"
                     "$import\n"
                     "  0:this\n"
                     "$end\n"
                     "\n"
                     "$typedef\n"
                     "  4:(6:i64,7:0:5):0:5\n"
                     "  5:(8:i64)\n"
                     "$end\n"
                     "\n"
                     "$type\n"
                     "  0:0:4\n"
                     "  1:0:5\n"
                     "$end\n"
                     "\n"
                     "$function\n"
                     "  0:0:var:0:reg:5(0:i64,1:i64,2:i64,3:type#0)->i64{\n"
                     "    label:entry\n"
                     "      reg#0 = arg,0\n"
                     "      reg#1 = arg,1\n"
                     "      reg#2 = iadd,i64,reg#0,reg#1\n"
                     "      reg#3 = arg,2\n"
                     "      reg#4 = iadd,i64,reg#2,reg#3\n"
                     "      ret,reg#4\n"
                     "    label:end\n"
                     "  }\n"
                     "$end";

    virtual_machine->pre_load_module("test", vm_code);
    auto* module = virtual_machine->load_module("test");
    if (module == nullptr) {
        printf("NULL!!\n");
    }

    auto* vm_thread = virtual_machine->create_thread(1024);

    auto* function = module->functions[0];
    size_t arguments_size = function->argument_types.size();
    auto* arguments = new uint64_t[arguments_size];

    int64_t argument1 = 20;
    int64_t argument2 = 30;
    int64_t argument3 = 40;
    arguments[0] = *((uint64_t*) &argument1);
    arguments[1] = *((uint64_t*) &argument2);
    arguments[2] = *((uint64_t*) &argument3);

    for (size_t s = 0; s < arguments_size; s++) {
        printf("argument : %llu | value : %lld\n", s, arguments[s]);
    }

    uint64_t result = virtual_machine->run_function(vm_thread, module, function, arguments);
    printf("Result = %llu\n", *((int64_t*) &result));

    std::cout << "Complete!\n";
}