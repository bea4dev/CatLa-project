#include <vm/modules/orders/GetField.h>
#include <heap/HeapAllocator.h>
#include <vm/modules/Module.h>
#include <vm/modules/util/BitSet.h>

GetObjectFieldOwnershipAtomic::GetObjectFieldOwnershipAtomic(size_t target_register, size_t result_register, size_t using_type_index, const string& field_name) {
    this->target_register = target_register;
    this->result_register = result_register;
    this->using_type_index = using_type_index;
    this->field_name = field_name;
    this->field_index = 0;
}

void GetObjectFieldOwnershipAtomic::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) {
    auto* parent_object = (HeapObject*) registers[this->target_register];

    object_lock(parent_object);
    auto* fields = (uint64_t*) (((uint8_t*) parent_object) + sizeof(HeapObject));
    auto* field_object = (HeapObject*) fields[this->field_index];
    if (field_object != nullptr) {
        field_object->count.fetch_add(1, std::memory_order_relaxed);
    }
    object_unlock(parent_object);

    registers[this->result_register] = (uint64_t) field_object;
}

void GetObjectFieldOwnershipAtomic::link(void* module, void* function) {
    auto* mod = (Module*) module;
    auto* type = mod->using_types[this->using_type_index];

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
