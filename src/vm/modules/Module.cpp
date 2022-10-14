#include <vm/modules/Module.h>
#include <utility>

Module::Module(string name, ConstValue* const_values, size_t const_values_size, const vector<string>& import_module_names, const vector<Type*>& type_defines, const vector<TypeInfo>& using_type_infos, const vector<Function*>& functions) {
    this->name = std::move(name);
    this->const_values = const_values;
    this->const_values_size = const_values_size;
    this->import_module_names = import_module_names;
    this->type_defines = type_defines;

    for (auto& it : type_defines) {
        this->type_define_map[it->type_name] = it;
    }

    this->using_type_infos = using_type_infos;
    this->functions = functions;
}
