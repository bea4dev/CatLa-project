#include "Type.h"

#include <utility>

using namespace modules;

Type::Type(std::string type_name, uint32_t runtime_type_id, size_t number_of_fields, Type* parent) {
    this->type_name = std::move(type_name);
    this->runtime_type_id = runtime_type_id;
    this->number_of_fields = number_of_fields;
    this->parent = parent;
}