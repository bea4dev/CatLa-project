#include "Type.h"

#include <utility>

using namespace modules;

Type::Type(std::string type_name, size_t runtime_type_id, size_t refs_length, size_t vals_length, vector<TypeInfo> parent_infos) {
    this->module = nullptr;
    this->type_name = std::move(type_name);
    this->runtime_type_id = runtime_type_id;
    this->refs_length = refs_length;
    this->vals_length = vals_length;
    this->parents_size = parent_infos.size();
    this->parent_infos = std::move(parent_infos);
    this->parents = nullptr;
}
