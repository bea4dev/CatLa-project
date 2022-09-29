#include <vm/modules/VMModule.h>

VMModule::VMModule(ConstValue *const_values, size_t const_values_size) {
    this->const_values = const_values;
    this->const_values_size = const_values_size;
}
