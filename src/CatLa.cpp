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
#include <string>
#include <regex>
#include <util/StringUtil.h>
#include <vm/parser/VMParser.h>

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
        //return (HeapObject*) malloc(40 + 16);
    }
    count++;
    c++;
    auto* parent = (size_t**) global_heap->malloc(nullptr, 2, 0, &j);
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

    auto* dummy_module = new Module("nyan", nullptr, 0, nullptr, 0);
    virtual_machine->register_module(dummy_module);

    string vm_code = u8"$const\n"
                     "  0:4:s#nyan\n"
                     "  1:13:s#catla::system\n"
                     "  2:13:s#catla::string\n"
                     "  3:13:s#catla::stdout\n"
                     "  4:16:s#catla::primitive\n"
                     "  5:12:s#Hello world!\n"
                     "  6:4:i#-128\n"
                     "  7:4:f#3.14\n"
                     "$end\n"
                     "$import\n"
                     "  0:this\n"
                     "  1:const#0\n"
                     "$end";
    auto* module = parser::parse("test", &vm_code);
    if (module != nullptr) {
        auto* const_values = module->const_values;
        size_t const_values_size = module->const_values_size;
        if (const_values != nullptr) {
            for (size_t index = 0; index < const_values_size; index++) {
                auto* const_value_reference = const_values + index;
                size_t byte_size = const_value_reference->byte_size;

                switch (const_value_reference->type) {
                    case 's': {
                        char* word_str = (char*) const_value_reference->value_reference;
                        char* str = new char[byte_size + 1];
                        for (size_t s = 0; s < byte_size; s++) {
                            str[s] = word_str[s];
                        }
                        str[byte_size] = '\0';
                        printf("const : string : %s\n", str);
                        delete[] str;
                        break;
                    }
                    case 'i': {
                        int64_t ci = *((int64_t*) const_value_reference->value_reference);
                        printf("const : int64 : %lld\n", ci);
                        break;
                    }
                    case 'f': {
                        double cf = *((double*) const_value_reference->value_reference);
                        printf("const : double : %f\n", cf);
                        break;
                    }
                    default: {
                        printf("const : UNKNOWN\n");
                    }
                }
            }
        } else {
            printf("Const values is null!\n");
        }

        auto** imports = module->imports;
        if (imports != nullptr) {
            size_t imports_size = module->imports_size;
            for (size_t s = 0; s < imports_size; s++) {
                auto *mod = imports[s];
                printf("import : %s\n", mod->name.c_str());
            }
        } else {
            printf("IMPORTS NULL!\n");
        }
    } else {
        printf("Module is null!\n");
    }


    std::cout << "Complete!\n";
}