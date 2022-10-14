#include "Type.h"

#include <utility>

using namespace modules;

Type::Type(PrimitiveType* primitive_type, std::string type_name, size_t runtime_type_id, const vector<FieldInfo>& field_infos, TypeInfo parent_info) {
    this->primitive_type = primitive_type;
    this->module = nullptr;
    this->type_name = std::move(type_name);
    this->runtime_type_id = runtime_type_id;
    this->field_infos = field_infos;
    this->parent_info = std::move(parent_info);
    this->parent = nullptr;
}
