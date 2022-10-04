#include <vm/modules/Module.h>
#include <utility>

Module::Module(string name, ConstValue* const_values, size_t const_values_size, vector<string> import_module_names, Type** type_defines, size_t type_defines_size, vector<TypeInfo> using_type_infos) {
    this->name = std::move(name);
    this->const_values = const_values;
    this->const_values_size = const_values_size;
    this->import_module_names = std::move(import_module_names);
    this->imports = nullptr;
    this->imports_size = 0;
    this->type_defines = type_defines;
    this->type_defines_size = type_defines_size;
    this->using_type_infos = std::move(using_type_infos);
    this->using_types = nullptr;
    this->using_types_size = 0;
}
