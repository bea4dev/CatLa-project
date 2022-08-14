#pragma once

#include <string>
#include <stdint.h>

namespace modules {

    class CatLaClass {

    public:
        std::string class_name;
        uint32_t runtime_class_id;
        size_t number_of_fields;

        CatLaClass(std::string class_name, uint32_t runtime_class_id, size_t number_of_fields);

    };


}
