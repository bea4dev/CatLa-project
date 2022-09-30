#include "NyanVM.h"
#include "Opcode.h"
#include <stack>

using namespace nyan;
using namespace heap;

namespace nyan {
    atomic_size_t thread_ids;
    size_t reserved_threads = 1;
}

VMThread::VMThread(size_t stack_size) {
    this->thread_id = thread_ids.fetch_add(1);
    this->stack_size = stack_size;
    uint8_t a = 0;
    this->top_of_stack_address = (size_t) &a;
}

VMThread::~VMThread() = default;


NyanVM::NyanVM() = default;

void NyanVM::run(VMThread* vm_thread, size_t thread_id, CodeBlock *code_block) {
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
                //auto* object = new TreeHeapObject(class_info, is_arc, thread_id, class_info->number_of_fields);
                //stack.push((size_t) object);
                break;
            }

            case opcode::heap_delete : {
                //auto* object = (TreeHeapObject*) stack.top();
                stack.pop();
                //object->unsafe_release();
                break;
            }

            case opcode::arc_hold : {
                //auto* object = (TreeHeapObject*) stack.top();
                //object->hold(thread_id);
                break;
            }

            case opcode::arc_drop : {
                //auto* object = (TreeHeapObject*) stack.top();
                //object->drop(thread_id);
                break;
            }

            case opcode::suspend : {
                //TODO
                break;
            }

            case opcode::get_object_field : {/*
                size_t index = stack.top();
                stack.pop();
                auto* object = (TreeHeapObject*) stack.top();
                stack.pop();
                size_t field_capacity = object->field_capacity;
                if (index >= field_capacity) {
                    throw "Out of capacity.";
                }
                stack.push((size_t) object->get_field_object(index));*/
                break;
            }

            case opcode::set_object_field : {/*
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
                object->set_field_object(field_object, field_object->runtime_object_id, index);*/
                break;
            }

            case opcode::get_module_field : {
                //TODO
                break;
            }

            case opcode::i8_add : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::i8_sub : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::i8_mul : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::i8_div : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::i8_rem : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::i8_l_shift : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::i8_r_shift : {
                auto i1 = *((int8_t*)&stack.top());
                stack.pop();
                auto i2 = *((int8_t*)&stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::i8_and : {
                auto i1 = *((uint8_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint8_t*)&stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::i8_or : {
                auto i1 = *((uint8_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint8_t*)&stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::i8_xor : {
                auto i1 = *((uint8_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint8_t*)&stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::i16_add : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::i16_sub : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::i16_mul : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::i16_div : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::i16_rem : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::i16_l_shift : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::i16_r_shift : {
                auto i1 = *((int16_t*)&stack.top());
                stack.pop();
                auto i2 = *((int16_t*)&stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::i16_and : {
                auto i1 = *((uint16_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint16_t*)&stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::i16_or : {
                auto i1 = *((uint16_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint16_t*)&stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::i16_xor : {
                auto i1 = *((uint16_t*)&stack.top());
                stack.pop();
                auto i2 = *((uint16_t*)&stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
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

            case opcode::u8_add : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::u8_sub : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::u8_mul : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::u8_div : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::u8_rem : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::u8_l_shift : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::u8_r_shift : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::u8_and : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::u8_or : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::u8_xor : {
                auto i1 = ((uint8_t)stack.top());
                stack.pop();
                auto i2 = ((uint8_t)stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::u16_add : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::u16_sub : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::u16_mul : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::u16_div : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::u16_rem : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::u16_l_shift : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::u16_r_shift : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::u16_and : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::u16_or : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::u16_xor : {
                auto i1 = ((uint16_t)stack.top());
                stack.pop();
                auto i2 = ((uint16_t)stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::u32_add : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::u32_sub : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::u32_mul : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::u32_div : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::u32_rem : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::u32_l_shift : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::u32_r_shift : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::u32_and : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::u32_or : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::u32_xor : {
                auto i1 = ((uint32_t)stack.top());
                stack.pop();
                auto i2 = ((uint32_t)stack.top());
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::u64_add : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 + i1);
                break;
            }

            case opcode::u64_sub : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 - i1);
                break;
            }

            case opcode::u64_mul : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 * i1);
                break;
            }

            case opcode::u64_div : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 / i1);
                break;
            }

            case opcode::u64_rem : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 % i1);
                break;
            }

            case opcode::u64_l_shift : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 << i1);
                break;
            }

            case opcode::u64_r_shift : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 >> i1);
                break;
            }

            case opcode::u64_and : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 & i1);
                break;
            }

            case opcode::u64_or : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 | i1);
                break;
            }

            case opcode::u64_xor : {
                auto i1 = stack.top();
                stack.pop();
                auto i2 = stack.top();
                stack.pop();
                stack.push(i2 ^ i1);
                break;
            }

            case opcode::f32_add : {
                auto i1 = *((float*)&stack.top());
                stack.pop();
                auto i2 = *((float*)&stack.top());
                stack.pop();
                auto result = i2 + i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f32_sub : {
                auto i1 = *((float*)&stack.top());
                stack.pop();
                auto i2 = *((float*)&stack.top());
                stack.pop();
                auto result = i2 - i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f32_mul : {
                auto i1 = *((float*)&stack.top());
                stack.pop();
                auto i2 = *((float*)&stack.top());
                stack.pop();
                auto result = i2 * i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f32_div : {
                auto i1 = *((float*)&stack.top());
                stack.pop();
                auto i2 = *((float*)&stack.top());
                stack.pop();
                auto result = i2 / i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f64_add : {
                auto i1 = *((double*)&stack.top());
                stack.pop();
                auto i2 = *((double*)&stack.top());
                stack.pop();
                auto result = i2 + i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f64_sub : {
                auto i1 = *((double*)&stack.top());
                stack.pop();
                auto i2 = *((double*)&stack.top());
                stack.pop();
                auto result = i2 - i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f64_mul : {
                auto i1 = *((double*)&stack.top());
                stack.pop();
                auto i2 = *((double*)&stack.top());
                stack.pop();
                auto result = i2 * i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

            case opcode::f64_div : {
                auto i1 = *((double*)&stack.top());
                stack.pop();
                auto i2 = *((double*)&stack.top());
                stack.pop();
                auto result = i2 / i1;
                stack.push(*((uint64_t*)&result));
                break;
            }

        }
    }

    printf("Stack last : %d\n", *((int32_t*)&stack.top()));
}

VMModule* NyanVM::get_module(const string& module_name) {
    if (this->loaded_module_map.find(module_name) != this->loaded_module_map.end()) {
        return this->loaded_module_map[module_name];
    }
    return nullptr;
}

void NyanVM::register_module(VMModule* module) {
    this->loaded_module_map[module->name] = module;
}
