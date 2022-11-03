#include <iostream>
#include "CatLa.h"
#include <random>
#include <vm/PrimitiveType.h>
#include <thread>
#include "vm/stack/StackUtil.h"
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

inline void set_object_field_atomic(HeapObject* parent, size_t field_index, HeapObject* field_object) {
    //field_object->count.fetch_add(1, std::memory_order_relaxed);

    object_lock(parent);
    auto* old_object = (HeapObject*) get_object_field(parent, field_index);
    set_object_field(parent, field_index, (uint64_t) field_object);
    object_unlock(parent);

    if (old_object != nullptr) {
        //old_object->count.fetch_sub(1, std::memory_order_release);
    }
}

inline HeapObject* get_object_field_atomic(HeapObject* parent, size_t field_index) {
    HeapObject* field_object;
    while (true) {
        object_lock(parent);
        field_object = (HeapObject *) get_object_field(parent, field_index);
        //field_object->count.fetch_add(1, std::memory_order_relaxed);
        object_unlock(parent);

        if (field_object != (HeapObject*) 1) {
            break;
        }
    }
    return field_object;
}


size_t j = 0;
size_t c = 0;

HeapObject* create(int count) {
    if (count > 20) {
        c++;
        return (HeapObject*) virtual_machine->get_heap_allocator()->malloc(nullptr, 2, &j);
        //return (HeapObject*) calloc(1, 40 + 16);
        //return (HeapObject*) malloc(40 + 16);
    }
    count++;
    c++;
    auto* parent = (size_t**) virtual_machine->get_heap_allocator()->malloc(nullptr, 2, &j);
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

HeapObject* heap_object;
size_t a = 0;
int b = 0;

void* func1(void* args) {
    for (int s = 0; s < 10000; s++) {
        object_lock(heap_object);
        b++;
        object_unlock(heap_object);
    }
    printf("OK!\n");
    return nullptr;
}

void* func2(void* args) {
    for (int s = 0; s < 10000; s++) {
        object_lock(heap_object);
        //printf("%d\n", b);
        object_unlock(heap_object);
    }
    printf("OK!\n");
    return nullptr;
}

bool f = true;
volatile size_t n = 0;
volatile size_t m = 0;
volatile atomic_size_t flag;

int main()
{
    std::cout << "Hello World!\n";

    setup_virtual_machine();

    size_t in = 0;
    heap_object = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(nullptr, 2, &in);

    pthread_t pthread1;
    pthread_t pthread2;
    pthread_t pthread3;
    pthread_t pthread4;
    pthread_attr_t thread_attribute;
    pthread_attr_init(&thread_attribute);
    pthread_create(&pthread1, &thread_attribute, func1, nullptr);
    pthread_create(&pthread2, &thread_attribute, func2, nullptr);
    pthread_create(&pthread3, &thread_attribute, func1, nullptr);
    pthread_create(&pthread4, &thread_attribute, func2, nullptr);
    pthread_join(pthread1, nullptr);
    pthread_join(pthread2, nullptr);
    pthread_join(pthread3, nullptr);
    pthread_join(pthread4, nullptr);
    printf("%d\n", b);

    Timing timing1;
    Timing timing2;

    timing1.start();
    for (int s = 0; s < 10000000; s++) {
        n++;
    }
    timing1.end();


    auto* allocator = virtual_machine->get_heap_allocator();
    timing2.start();
    /*
    for (int s = 0; s < 10000000; s++) {
        while (true) {
            size_t previous = flag.exchange(1, std::memory_order_relaxed);
            if (previous != 1) {
                break;
            }
        }
        m++;
        flag.exchange(0, std::memory_order_relaxed);
    }*/

    auto* list1 = (HeapObject*) calloc(1, sizeof(HeapObject) + (1000000 * 8));
    auto* list2 = (HeapObject*) calloc(1, sizeof(HeapObject) + (1000000 * 8));

    size_t at = 0;
    //auto* object1 = (HeapObject*) allocator->malloc(nullptr, 0, &at);
    //auto* object2 = (HeapObject*) allocator->malloc(nullptr, 0, &at);

    for (size_t v = 0; v < 1000000; v++) {
        auto* object1 = (HeapObject*) allocator->malloc(nullptr, 0, &at);
        auto* object2 = (HeapObject*) allocator->malloc(nullptr, 0, &at);
        //object1->count.fetch_add(1, std::memory_order_relaxed);
        //object2->count.fetch_add(1, std::memory_order_relaxed);
        set_object_field_atomic(list1, v, object1);
        set_object_field_atomic(list2, v, object2);
    }

    for (size_t v = 0; v < 1000000; v++) {
        auto* object1 = get_object_field_atomic(list1, v);
        auto* object2 = get_object_field_atomic(list2, v);
        set_object_field_atomic(list1, v, object2);
        set_object_field_atomic(list2, v, object1);
        //object1->count.fetch_sub(1, std::memory_order_relaxed);
        //object2->count.fetch_sub(1, std::memory_order_relaxed);
    }

    /*
    for (int s = 0; s < 10000000; s++) {
        if (f) {
            object_lock(heap_object);
            m++;
            object_unlock(heap_object);
        }
    }*/
    timing2.end();

    printf("%llu[ms]\n", timing1.get_sum_time());
    printf("list %llu[ms]\n", timing2.get_sum_time());

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