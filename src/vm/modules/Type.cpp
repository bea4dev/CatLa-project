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
    this->reference_fields = nullptr;
    this->is_cycling_type.store(false, std::memory_order_release);
}

Type* modules::get_from_primitive_type(PrimitiveType* type) {
    if (type == nullptr) {
        return nullptr;
    }
    return (Type*) type->type;
}
