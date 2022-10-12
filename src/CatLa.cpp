#include <iostream>
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
                     "  0:4:s#test\n"
                     "$end\n"
                     "\n"
                     "$import\n"
                     "  0:this\n"
                     "$end\n"
                     "\n"
                     "$typedef\n"
                     "$end\n"
                     "\n"
                     "$type\n"
                     "$end\n"
                     "\n"
                     "$function\n"
                     "  0:0:var:0:reg:5(0:i64, 1:i64, 2:i64)->i64{\n"
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

        auto import_module_names = module->import_module_names;
        for (auto& import_module_name : import_module_names) {
            printf("module : %s\n", import_module_name.c_str());
        }

        auto** type_defines = module->type_defines;
        size_t type_defines_size = module->type_defines_size;
        if (type_defines != nullptr) {
            for (size_t s = 0; s < type_defines_size; s++) {
                auto* type = type_defines[s];
                printf("type define : %s %llu %llu\n", type->type_name.c_str(), type->refs_length, type->vals_length);
            }
        } else {
            printf("Type defines is null!\n");
        }

        auto using_type_infos = module->using_type_infos;
        for (auto& type_info : using_type_infos) {
            printf("using type : %llu %llu\n", type_info.import_index, type_info.type_define_index);
        }
    } else {
        printf("Module is null!\n");
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