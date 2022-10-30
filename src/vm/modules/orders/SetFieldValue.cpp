#include <vm/modules/orders/SetFieldValue.h>
#include <heap/HeapAllocator.h>
#include <vm/modules/Module.h>
#include <vm/modules/util/BitSet.h>

SetObjectFieldOwnership::SetObjectFieldOwnership(size_t target_register, size_t field_object_register_index, size_t using_type_index, const string& field_name, size_t field_index, bool borrow_lock) {
    this->target_register = target_register;
    this->field_object_register_index = field_object_register_index;
    this->using_type_index = using_type_index;
    this->field_name = field_name;
    this->field_index = field_index;
    this->borrow_lock = borrow_lock;
}

void SetObjectFieldOwnership::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) {
    auto* parent_object = (HeapObject*) registers[this->target_register];
    auto* fields = (uint64_t*) (((uint8_t*) parent_object) + sizeof(HeapObject));
    auto object_ptr = registers[this->field_object_register_index];

    if (this->borrow_lock) {
        while (true) {
            object_lock(parent_object);
            auto* field_object = (HeapObject*) fields[this->field_index];
            if (field_object != (HeapObject*) 1) {
                fields[this->field_index] = object_ptr;
                object_unlock(parent_object);
                break;
            }
            object_unlock(parent_object);
        }
    } else {
        object_lock(parent_object);
        fields[this->field_index] = object_ptr;
        object_unlock(parent_object);
    }
}

void SetObjectFieldOwnership::link(void* module, void* function) {
    if (!this->field_name.empty()) {
        auto *mod = (Module *) module;
        auto *type = mod->using_types[this->using_type_index];

        for (size_t i = 0; i < type->all_fields.size(); i++) {
            auto field = type->all_fields[i];
            if (field.name == this->field_name) {
                if (!get_flag(type->reference_fields, i)) {
                    //TODO - error handling
                    printf("NOT ARC FIELD!\n");
                }
                this->field_index = i;
                return;
            }
        }

        //TODO - error handling
    }
}
