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
#include <ctime>

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

inline void set_clone_object_field(CycleCollector* cycle_collector, HeapObject* parent, size_t field_index, HeapObject* field_object) {
    increase_reference_count(field_object);
    auto** field_ptr = ((HeapObject**) (parent + 1)) + field_index;
    HeapObject* old_field_object;
    while (true) {
        object_lock(parent);
        old_field_object = *field_ptr;
        if (old_field_object != (HeapObject*) 1) {
            *field_ptr = field_object;
            object_unlock(parent);
            break;
        }
        object_unlock(parent);
    }
    if (old_field_object != nullptr) {
        decrease_reference_count(cycle_collector, old_field_object);
    }
}

inline HeapObject* get_clone_object_field(HeapObject* parent, size_t field_index) {
    auto** field_ptr = ((HeapObject**) (parent + 1)) + field_index;
    while (true) {
        object_lock(parent);
        auto* field_object = *field_ptr;
        if (field_object != (HeapObject*) 1) {
            if (field_object != nullptr) {
                increase_reference_count(field_object);
            }
            object_unlock(parent);
            return field_object;
        }
        object_unlock(parent);
    }
}

inline void set_move_object_field(CycleCollector* cycle_collector, HeapObject* parent, size_t field_index, HeapObject* field_object) {
    auto** field_ptr = ((HeapObject**) (parent + 1)) + field_index;
    HeapObject* old_field_object;
    while (true) {
        object_lock(parent);
        old_field_object = *field_ptr;
        if (old_field_object != (HeapObject*) 1) {
            *field_ptr = field_object;
            object_unlock(parent);
            break;
        }
        object_unlock(parent);
    }
    if (old_field_object != nullptr) {
        decrease_reference_count(cycle_collector, old_field_object);
    }
}

inline HeapObject* get_move_object_field(HeapObject* parent, size_t field_index) {
    auto** field_ptr = ((HeapObject**) (parent + 1)) + field_index;
    HeapObject* field_object;
    while (true) {
        atomic_thread_fence(std::memory_order_release);
        object_lock(parent);
        field_object = *field_ptr;
        if (field_object != (HeapObject*) 1) {
            *field_ptr = nullptr;
            object_unlock(parent);
            break;
        }
        object_unlock(parent);
    }
    return field_object;
}

inline void set_move_object_field_uncheck(HeapObject* parent, size_t field_index, HeapObject* field_object) {
    auto** field_ptr = ((HeapObject**) (parent + 1)) + field_index;
    object_lock(parent);
    *field_ptr = field_object;
    object_unlock(parent);
}


size_t j = 0;
size_t c = 0;

Type* object_type1;
Type* object_type2;
Type* module_type;
vector<HeapObject*> created_objects;

HeapObject* create(int count) {
    if (count > 20) {
        c++;
        auto* obj = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(object_type1, 2, &j);
        created_objects.push_back(obj);
        return obj;
        //return (HeapObject*) calloc(1, 40 + 16);
        //return (HeapObject*) malloc(40 + 16);
    }
    count++;
    c++;
    auto* parent = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(object_type1, 2, &j);
    //auto* parent = (size_t**) calloc(1, 40 + 16);
    //auto* parent = (size_t**) malloc(40 + 16);
    auto* child1 = create(count);
    auto* child2 = create(count);
    created_objects.push_back(child1);
    created_objects.push_back(child2);

    set_move_object_field_uncheck(parent, 0, child1);
    set_move_object_field_uncheck(parent, 1, child2);

    return (HeapObject*) parent;
}

using namespace benchmark;

HeapObject* heap_object;
size_t a = 0;
int b = 0;

RWLock rw_lock;

HeapObject* module_object;

inline long long get_current_ms() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

size_t rand_map[10] = {};

void create_random_map() {
    for (auto& s : rand_map) {
        s = random();
    }
}

inline size_t get_10() {
    return clock() % 10;
}

inline HeapObject* create_object() {
    auto* object = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(object_type2, 2, &j);
    created_objects.push_back(object);
    return object;
}

atomic_bool task_flag(false);

void* func1(void* args) {
    printf("START!\n");
    auto* thread = virtual_machine->create_thread(2048);
    for (size_t s = 0; s < 100000; s++) {
        if (get_10() % 2 == 0) {
            auto* obj1 = (HeapObject*) thread->heap_allocator->malloc(object_type2, 3, &thread->allocator_search_start_index);
            auto* obj2 = (HeapObject*) thread->heap_allocator->malloc(object_type2, 3, &thread->allocator_search_start_index);
            auto* obj3 = (HeapObject*) thread->heap_allocator->malloc(object_type2, 3, &thread->allocator_search_start_index);
            object_lock(module_object);
            created_objects.push_back(obj1);
            created_objects.push_back(obj2);
            created_objects.push_back(obj3);
            object_unlock(module_object);
            set_move_object_field(virtual_machine->get_cycle_collector(), module_object, get_10(), obj1);
            set_move_object_field(virtual_machine->get_cycle_collector(), module_object, get_10(), obj2);
            set_move_object_field(virtual_machine->get_cycle_collector(), module_object, get_10(), obj3);
        } else {
            auto* obj1 = get_clone_object_field(module_object, get_10());
            auto* obj2 = get_clone_object_field(module_object, get_10());
            auto* obj3 = get_clone_object_field(module_object, get_10());
            if (get_10() % 2 == 0) {
                set_clone_object_field(virtual_machine->get_cycle_collector(), obj1, get_10() % 2, obj2);
                set_move_object_field(virtual_machine->get_cycle_collector(), obj2, get_10() % 2, obj3);
                decrease_reference_count(virtual_machine->get_cycle_collector(), obj1);
                decrease_reference_count(virtual_machine->get_cycle_collector(), obj2);
                //decrease_reference_count(virtual_machine->get_cycle_collector(), obj3);
            } else {
                set_clone_object_field(virtual_machine->get_cycle_collector(), obj1, get_10() % 2, obj2);
                set_clone_object_field(virtual_machine->get_cycle_collector(), obj2, get_10() % 2, obj3);
                set_clone_object_field(virtual_machine->get_cycle_collector(), obj3, get_10() % 2, obj1);
                decrease_reference_count(virtual_machine->get_cycle_collector(), obj1);
                decrease_reference_count(virtual_machine->get_cycle_collector(), obj2);
                decrease_reference_count(virtual_machine->get_cycle_collector(), obj3);
            }
        }
    }
    printf("END!\n");
    return nullptr;
}

void* func2(void* args) {
    while (true) {
        if (task_flag.load(std::memory_order_acquire)) {
            break;
        }
        //this_thread::sleep_for(std::chrono::milliseconds(5000));
        printf("CONCURRENT COLLECT START!\n");
        virtual_machine->get_cycle_collector()->collect_cycles();
        printf("CONCURRENT COLLECT END!\n");
    }
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

    create_random_map();

    size_t in = 0;
    heap_object = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(nullptr, 2, &in);

    object_type1 = new Type(nullptr, "TestClass", 0, {}, {});
    object_type2 = new Type(nullptr, "TestClass", 0, {}, {});
    module_type = new Type(nullptr, "TestModule", 0, {}, {});
    object_type2->is_cycling_type.store(true, std::memory_order_relaxed);
    auto* bits = create_bitset(2);
    set_flag(bits, 0, true);
    set_flag(bits, 1, true);
    object_type1->reference_fields = bits;
    object_type2->reference_fields = bits;

    bits = create_bitset(10);
    for (size_t s = 0; s < 10; s++) {
        set_flag(bits, s, true);
    }
    module_type->reference_fields = bits;


    module_object = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(module_type, 10, &j);
    set_move_object_field_uncheck(module_object, 0, create_object());
    set_move_object_field_uncheck(module_object, 1, create_object());
    set_move_object_field_uncheck(module_object, 2, create_object());
    set_move_object_field_uncheck(module_object, 3, create_object());
    set_move_object_field_uncheck(module_object, 4, create_object());
    set_move_object_field_uncheck(module_object, 5, create_object());
    set_move_object_field_uncheck(module_object, 6, create_object());
    set_move_object_field_uncheck(module_object, 7, create_object());
    set_move_object_field_uncheck(module_object, 8, create_object());
    set_move_object_field_uncheck(module_object, 9, create_object());

    Timing gc_timing;
    gc_timing.start();
    pthread_t pthread1;
    pthread_t pthread2;
    pthread_t pthread3;
    pthread_t pthread4;
    pthread_attr_t thread_attribute;
    pthread_attr_init(&thread_attribute);

    pthread_create(&pthread1, &thread_attribute, func1, nullptr);
    pthread_create(&pthread2, &thread_attribute, func1, nullptr);
    pthread_create(&pthread3, &thread_attribute, func1, nullptr);
    //pthread_create(&pthread4, &thread_attribute, func2, nullptr);
    pthread_join(pthread1, nullptr);
    pthread_join(pthread2, nullptr);
    pthread_join(pthread3, nullptr);

    task_flag.store(true, std::memory_order_release);

    //pthread_join(pthread4, nullptr);

    unordered_map<HeapObject*, size_t> count_cache_map;
    unordered_map<HeapObject*, size_t> flag_cache_map;
    for (auto& object : created_objects) {
        count_cache_map[object] = object->count.load(std::memory_order_acquire);
        flag_cache_map[object] = object->flag.load(std::memory_order_acquire);
    }
    //func1(nullptr);
    decrease_reference_count(virtual_machine->get_cycle_collector(), module_object);
    printf("COLLECT START!\n");
    virtual_machine->get_cycle_collector()->collect_cycles();
    printf("COLLECT END!\n");
    //printf("START! %llu %llu\n", get_10_random(), get_10());

    vector<HeapObject*> leaked_objects;

    for (auto& object : created_objects) {
        size_t object_flag = object->flag.load(std::memory_order_acquire);
        if (object_flag != 0) {
            bool is_white = false;
            for (auto& white_object : virtual_machine->get_cycle_collector()->white_objects) {
                if (white_object == object) {
                    is_white = true;
                    break;
                }
            }
            bool is_dec = false;
            for (auto& dec_object : virtual_machine->get_cycle_collector()->dec_objects) {
                if (dec_object == object) {
                    is_dec = true;
                    break;
                }
            }
            const char* white = is_white ? "true" : "false";
            const char* dec = is_dec ? "true" : "false";

            const char* is_new_field_obj = object->field_length == 3 ? "true" : "false";
            printf("NOT DEAD! %s : %llu(%llu) : %llu(%llu) : %s : %s : [%p] ", is_new_field_obj, object->count.load(std::memory_order_acquire), count_cache_map[object], object_flag, flag_cache_map[object], white, dec, object);

            auto** fields = (HeapObject**) (object + 1);
            auto* object_type = (Type*) object->type_info;
            size_t field_length = object->field_length;
            for (size_t s = 0; s < field_length; s++) {
                printf("%p ", fields[s]);
            }
            printf("\n");
            leaked_objects.push_back(object);
        } else {
            //printf("DEAD!\n");
        }
    }

    printf("--- TRACE INFO ---\n");
    for (auto& object : created_objects) {
        auto** fields = (HeapObject**) (object + 1);
        auto* object_type = (Type*) object->type_info;
        size_t field_length = object->field_length;
        for (size_t s = 0; s < field_length; s++) {
            auto* field = fields[s];
            for (auto& leaked_object : leaked_objects) {
                if (field == leaked_object) {
                    goto break_field_loop;
                }
            }
        }
        continue;

        break_field_loop:

        size_t object_flag = object->flag.load(std::memory_order_acquire);

        bool is_white = false;
        for (auto& white_object : virtual_machine->get_cycle_collector()->white_objects) {
            if (white_object == object) {
                is_white = true;
                break;
            }
        }
        bool is_dec = false;
        for (auto& dec_object : virtual_machine->get_cycle_collector()->dec_objects) {
            if (dec_object == object) {
                is_dec = true;
                break;
            }
        }
        const char* white = is_white ? "true" : "false";
        const char* dec = is_dec ? "true" : "false";

        printf("%llu(%llu) : %llu(%llu) : %s : %s : [%p] ", object->count.load(std::memory_order_acquire), count_cache_map[object], object_flag, flag_cache_map[object], white, dec, object);
        for (size_t s = 0; s < field_length; s++) {
            printf("%p ", fields[s]);
        }
        printf("\n");
    }
    printf("------------------\n");

    /*
    auto* t1 = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(object_type2, 2, &j);
    auto* t2 = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(object_type2, 2, &j);
    auto* parent = (HeapObject*) virtual_machine->get_heap_allocator()->malloc(object_type1, 2, &j);
    created_objects.push_back(t1);
    created_objects.push_back(t2);
    created_objects.push_back(parent);
    set_move_object_field(virtual_machine->get_cycle_collector(), parent, 0, t1);
    set_move_object_field(virtual_machine->get_cycle_collector(), parent, 1, t2);
    set_clone_object_field(virtual_machine->get_cycle_collector(), t1, 0, t2);
    set_clone_object_field(virtual_machine->get_cycle_collector(), t2, 0, t1);
    set_clone_object_field(virtual_machine->get_cycle_collector(), t1, 1, t1);
    set_clone_object_field(virtual_machine->get_cycle_collector(), t2, 1, t2);
    decrease_reference_count(virtual_machine->get_cycle_collector(), parent);
    for (auto& object : created_objects) {
        if (object->flag.load(std::memory_order_acquire) != 0) {
            printf("NOT DEAD! : %llu\n", object->count.load(std::memory_order_acquire));
        } else {
            printf("DEAD!\n");
        }
    }
    virtual_machine->get_cycle_collector()->collect_cycles();
    printf("COLLECT OK!\n");
    for (auto& object : created_objects) {
        if (object->flag.load(std::memory_order_acquire) != 0) {
            printf("NOT DEAD! : %llu\n", object->count.load(std::memory_order_acquire));
        } else {
            printf("DEAD!\n");
        }
    }*/
    gc_timing.end();
    printf("GC : %llu[ms]\n", gc_timing.get_sum_time());

    Timing timing1;
    Timing timing2;

    RWLock rwLock;
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