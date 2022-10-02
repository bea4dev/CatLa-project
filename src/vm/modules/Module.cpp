#include <vm/modules/Module.h>

#include <utility>

Module::Module(string name, ConstValue *const_values, size_t const_values_size, Module** imports, size_t imports_size) {
    this->name = std::move(name);
    this->const_values = const_values;
    this->const_values_size = const_values_size;
    this->imports = imports;
    this->imports_size = imports_size;
}
