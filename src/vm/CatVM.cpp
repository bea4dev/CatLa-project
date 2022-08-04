#include "CatVM.h"
#include "Opcode.h"
#include <stack>

using namespace cat_vm;

namespace cat_vm {
    atomic_size_t thread_ids;
}

VM_Thread::VM_Thread() {
    this->thread_id = thread_ids.fetch_add(1);
}

VM_Thread::~VM_Thread() = default;


CatVM::CatVM() = default;

void CatVM::run(const CodeBlock* code_block) {
    this->threads_manage_lock.lock();
    auto* current_thread = new VM_Thread();
    this->threads.push_back(current_thread);
    this->threads_manage_lock.unlock();

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
    for (size_t i = 0; i < byte_code_length; i++) {
        uint8_t opcode = byte_code[i];

        switch (opcode) {

            case opcode::set_info : {
                //TODO
                break;
            }

            case opcode::push_const : {
                size_t index = byte_code[i + 1] << 8;
                index |= byte_code[i + 2];

                uint64_t value = const_values[index];
                //printf("push_const : %zu, %d\n", index, value);
                stack.push(value);

                i += 2;
                break;
            }

            case opcode::set_reg : {
                //TODO
                break;
            }

            case opcode::push_reg : {
                //TODO
                break;
            }

            case opcode::heap_new : {
                //TODO
                break;
            }

            case opcode::heap_delete : {
                //TODO
                break;
            }

            case opcode::arc_new : {
                //TODO
                break;
            }

            case opcode::arc_hold : {
                //TODO
                break;
            }

            case opcode::arc_drop : {
                //TODO
                break;
            }

            case opcode::suspend : {
                //TODO
                break;
            }

            case opcode::get_object_field : {
                //TODO
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

            case opcode::i32_add : {
                auto i1 = *((int32_t *)&stack.top());
                stack.pop();

                auto i2 = *((int32_t *)&stack.top());
                stack.pop();

                //printf("i32_add : %d\n", i2 + i1);

                stack.push(i2 + i1);
                break;
            }

            case opcode::i32_sub : {
                auto i1 = *((int32_t *)&stack.top());
                stack.pop();

                auto i2 = *((int32_t *)&stack.top());
                stack.pop();

                //printf("i32_sub : %d\n", i2 - i1);

                stack.push(i2 - i1);
                break;
            }

            case opcode::i32_mul : {
                auto i1 = *((int32_t *)&stack.top());
                stack.pop();

                auto i2 = *((int32_t *)&stack.top());
                stack.pop();

                //printf("i32_mul : %d\n", i2 * i1);

                stack.push(i2 * i1);
                break;
            }

            case opcode::i32_div : {
                auto i1 = *((int32_t *)&stack.top());
                stack.pop();

                auto i2 = *((int32_t *)&stack.top());
                stack.pop();

                //printf("i32_div : %d\n", i2 / i1);

                stack.push(i2 / i1);
                break;
            }

            case opcode::i32_rem : {
                auto i1 = *((int32_t *)&stack.top());
                stack.pop();

                auto i2 = *((int32_t *)&stack.top());
                stack.pop();

                //printf("i32_rem : %d\n", i1 % i2);

                stack.push(i1 % i2);
                break;
            }

            case opcode::i64_add : {
                break;
            }

            case opcode::i64_sub : {
                break;
            }

            case opcode::i64_mul : {
                break;
            }

            case opcode::i64_div : {
                break;
            }

            case opcode::i64_rem : {
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

    printf("Stack last : %d\n", *((int32_t *)&stack.top()));
}