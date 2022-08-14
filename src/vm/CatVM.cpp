#include "CatVM.h"
#include "Opcode.h"
#include <stack>
#include <heap/HeapObject.h>
#include <heap/HeapManager.h>

using namespace cat_vm;
using namespace heap;

namespace cat_vm {
    atomic_size_t thread_ids;
    size_t reserved_threads = 1;
}

VM_Thread::VM_Thread() {
    this->thread_id = thread_ids.fetch_add(1);
}

VM_Thread::~VM_Thread() = default;


CatVM::CatVM() = default;

void CatVM::run(CodeBlock *code_block) {
    this->threads_manage_lock.lock();
    auto* current_thread = new VM_Thread();
    this->threads.push_back(current_thread);
    this->threads_manage_lock.unlock();
    size_t thread_id = current_thread->thread_id;

    CodeBlock* current_code_block = code_block;
    vector<uint8_t> byte_code = *code_block->byte_code;
    vector<uint64_t> const_values = *code_block->const_values;
    size_t byte_code_length = byte_code.size();


    printf("\nConst value :\n");
    for (auto it = const_values.begin(); it != const_values.end(); ++it) {
        uint64_t value = (*it);
        printf("%016llx\n", value);
    }
    printf("\nByte code :\n");
    for (size_t i = 0; i < byte_code_length; i++) {
        uint8_t opcode = byte_code[i];
        printf("%02x ", opcode);

        if (i != 0 && i % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n\n");

    stack<uint64_t> stack;
    auto* reg = new uint64_t[code_block->reg_size];
    for (size_t i = 0; i < byte_code_length; i++) {
        uint8_t opcode = byte_code[i];

        switch (opcode) {

            case opcode::set_info : {
                //TODO
                break;
            }

            case opcode::push_const : {
                size_t index = ((size_t) byte_code[++i]) << 8;
                index |= ((size_t) byte_code[++i]);
                uint64_t value = const_values[index];
                stack.push(value);
                break;
            }

            case opcode::set_reg : {
                size_t index = ((size_t) byte_code[++i]) << 8;
                index |= ((size_t) byte_code[++i]);
                reg[index] = stack.top();
                stack.pop();
                break;
            }

            case opcode::push_reg : {
                size_t index = ((size_t) byte_code[++i]) << 8;
                index |= ((size_t) byte_code[++i]);
                stack.push(reg[index]);
                break;
            }

            case opcode::heap_new : {
                auto* class_info = (CatLaClass*) stack.top();
                stack.pop();
                uint8_t option = byte_code[++i];
                bool is_arc = (option & 1) != 0;
                auto* object = new TreeHeapObject(class_info, is_arc, thread_id, class_info->number_of_fields);
                stack.push((size_t) object);
                break;
            }

            case opcode::heap_delete : {
                auto* object = (TreeHeapObject*) stack.top();
                stack.pop();
                object->unsafe_release();
                break;
            }

            case opcode::arc_hold : {
                auto* object = (TreeHeapObject*) stack.top();
                object->hold(thread_id);
                break;
            }

            case opcode::arc_drop : {
                auto* object = (TreeHeapObject*) stack.top();
                object->drop(thread_id);
                break;
            }

            case opcode::suspend : {
                //TODO
                break;
            }

            case opcode::get_object_field : {
                size_t index = stack.top();
                stack.pop();
                auto* object = (TreeHeapObject*) stack.top();
                stack.pop();
                size_t field_capacity = object->field_capacity;
                if (index >= field_capacity) {
                    throw "Out of capacity.";
                }
                stack.push((size_t) object->get_field_object(index));
                break;
            }

            case opcode::set_object_field : {
                size_t index = stack.top();
                stack.pop();
                auto* field_object = (HeapObject*) stack.top();
                stack.pop();
                auto* object = (TreeHeapObject*) stack.top();
                stack.pop();
                size_t field_capacity = object->field_capacity;
                if (index >= field_capacity) {
                    throw "Out of capacity.";
                }
                object->set_field_object(field_object, field_object->runtime_object_id, index);
                break;
            }

            case opcode::get_module_field : {
                //TODO
                break;
            }

            case opcode::i8_add : {
                break;
            }

            case opcode::i8_sub : {
                break;
            }

            case opcode::i8_mul : {
                break;
            }

            case opcode::i8_div : {
                break;
            }

            case opcode::i8_rem : {
                break;
            }

            case opcode::i8_l_shift : {
                break;
            }

            case opcode::i8_r_shift : {
                break;
            }

            case opcode::i8_ur_shift : {
                break;
            }

            case opcode::i8_and : {
                break;
            }

            case opcode::i8_or : {
                break;
            }

            case opcode::i8_xor : {
                break;
            }

            case opcode::i16_add : {
                break;
            }

            case opcode::i16_sub : {
                break;
            }

            case opcode::i16_mul : {
                break;
            }

            case opcode::i16_div : {
                break;
            }

            case opcode::i16_rem : {
                break;
            }

            case opcode::i16_l_shift : {
                break;
            }

            case opcode::i16_r_shift : {
                break;
            }

            case opcode::i16_ur_shift : {
                break;
            }

            case opcode::i16_and : {
                break;
            }

            case opcode::i16_or : {
                break;
            }

            case opcode::i16_xor : {
                break;
            }

            case opcode::i32_add : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::i32_sub : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::i32_mul : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::i32_div : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::i32_rem : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::i32_l_shift : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::i32_r_shift : {
                auto i1 = *((int32_t*)&stack.top());
                stack.pop();
                auto i2 = *((int32_t*)&stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::i32_ur_shift : {
                auto i1 = *((uint32_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint32_t*)&stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::i32_and : {
                auto i1 = *((uint32_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint32_t*)&stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::i32_or : {
                auto i1 = *((uint32_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint32_t*)&stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::i32_xor : {
                auto i1 = *((uint32_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint32_t*)&stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::i64_add : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::i64_sub : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::i64_mul : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::i64_div : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::i64_rem : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::i64_l_shift : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::i64_r_shift : {
                auto i1 = *((int64_t*)&stack.top());
                stack.pop();
                auto i2 = *((int64_t*)&stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::i64_ur_shift : {
                auto i1 = *((uint64_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint64_t*)&stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::i64_and : {
                auto i1 = *((uint64_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint64_t*)&stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::i64_or : {
                auto i1 = *((uint64_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint64_t*)&stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::i64_xor : {
                auto i1 = *((uint64_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint64_t*)&stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::f32_add : {
                break;
            }

            case opcode::f32_sub : {
                break;
            }

            case opcode::f32_mul : {
                break;
            }

            case opcode::f32_div : {
                break;
            }

            case opcode::f32_rem : {
                break;
            }

            case opcode::f64_add : {
                break;
            }

            case opcode::f64_sub : {
                break;
            }

            case opcode::f64_mul : {
                break;
            }

            case opcode::f64_div : {
                break;
            }

            case opcode::f64_rem : {
                break;
            }

        }
    }

    printf("Stack last : %d\n", *((int32_t*)&stack.top()));
}