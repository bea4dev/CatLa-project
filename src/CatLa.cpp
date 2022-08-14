#include <iostream>
#include "CatLa.h"
#include "heap/HeapManager.h"
#include <random>
#include "vm/Opcode.h"
#include <thread>

CatVM* virtual_machine = nullptr;
HeapManager* heap_manager = nullptr;

void setup_virtual_machine() {
    static_runtime_object_id.fetch_add(1);
    virtual_machine = new CatVM();
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
    virtual_machine->run(code_block);

    std::cout << "Complete!\n";
}