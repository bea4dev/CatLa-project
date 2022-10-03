#pragma once

#include <string>
#include <cstdint>
#include <vm/modules/Module.h>
#include <vm/parser/VMParser.h>

namespace modules {

    class Type {

    public:
        Module* module;
        std::string type_name;
        size_t runtime_type_id;
        size_t refs_length;
        size_t vals_length;
        vector<TypeInfo> parent_infos;
        Type** parents;
        size_t parents_size;

        Type(std::string type_name, size_t runtime_type_id, size_t refs_length, size_t vals_length, vector<TypeInfo> parent_infos);

    };


}
