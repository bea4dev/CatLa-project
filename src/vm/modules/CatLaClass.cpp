#include "CatLaClass.h"

#include <utility>

using namespace modules;

CatLaClass::CatLaClass(std::string class_name, uint32_t runtime_class_id, uint32_t number_of_fields) {
    this->class_name = std::move(class_name);
    this->runtime_class_id = runtime_class_id;
    this->number_of_fields = number_of_fields;
}