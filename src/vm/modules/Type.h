#pragma once

#include <string>
#include <cstdint>

namespace modules {

    class Type {

    public:
        std::string type_name;
        uint32_t runtime_type_id;
        size_t number_of_fields;
        Type* parent;

        Type(std::string type_name, uint32_t runtime_type_id, size_t number_of_fields, Type* parent);

    };


}
