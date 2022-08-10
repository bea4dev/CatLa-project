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
    vector<uint64_t> reg;
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
                stack.push(value);
                i += 2;
                break;
            }

            case opcode::set_reg : {
                size_t index = byte_code[i + 1] << 8;
                index |= byte_code[i + 2];

                size_t reg_length = reg.size();
                if (reg_length <= index) {
                    for (size_t ri = 0; ri < reg_length - index + 1; ri++) {
                        reg.push_back(0);
                    }
                }

                reg[index] = stack.top();
                stack.pop();
                i += 2;
                break;
            }

            case opcode::push_reg : {
                size_t index = byte_code[i + 1] << 8;
                index |= byte_code[i + 2];
                stack.push(reg[index]);
                i += 2;
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