#include <iostream>
#include "CatLa.h"
#include "heap/HeapManager.h"
#include <random>
#include "vm/Opcode.h"
#include <thread>
#include <vm/stack/stack.h>
#include <pthread.h>

NyanVM* virtual_machine = nullptr;
HeapManager* heap_manager = nullptr;

void setup_virtual_machine() {
    static_runtime_object_id.fetch_add(1);
    virtual_machine = new NyanVM();
    heap_manager = new heap::HeapManager((size_t) 8);
    reserved_threads = (size_t) thread::hardware_concurrency();
}


int random() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 10000);
    return dist(gen);
}

void release(TreeHeapObject* object) {
    object->hold(0);
    object->drop(0);
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

int main()
{
    std::cout << "Hello World!\n";

    setup_virtual_machine();

    auto* byte_code = new vector<uint8_t>;
    auto* const_values = new vector<uint64_t>;

    const_values->push_back(20);
    const_values->push_back(30);
    const_values->push_back(40);
    const_values->push_back(15);

    byte_code->push_back(opcode::push_const);
    byte_code->push_back(0x00);
    byte_code->push_back(0x00);
    byte_code->push_back(opcode::push_const);
    byte_code->push_back(0x00);
    byte_code->push_back(0x01);
    byte_code->push_back(opcode::push_const);
    byte_code->push_back(0x00);
    byte_code->push_back(0x02);
    byte_code->push_back(opcode::i32_mul);
    byte_code->push_back(opcode::i32_add);
    byte_code->push_back(opcode::push_const);
    byte_code->push_back(0x00);
    byte_code->push_back(0x03);
    byte_code->push_back(opcode::i32_sub);

    auto* code_block = new CodeBlock(byte_code, const_values, 20);
    auto* thread = new VMThread();
    NyanVM::run(thread, thread->thread_id, code_block);

    pthread_attr_t thread_attribute;
    pthread_t pthread1;
    stack_size = PTHREAD_STACK_MIN + 0x8000;

    pthread_attr_init(&thread_attribute);
    pthread_attr_setstacksize(&thread_attribute, stack_size);
    pthread_create(&pthread1, &thread_attribute, start, nullptr);
    pthread_join(pthread1, nullptr);


    std::cout << "Complete!\n";
}