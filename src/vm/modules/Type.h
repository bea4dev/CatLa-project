#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <vm/parser/Structs.h>

using namespace std;

namespace modules {

    class Type {

    public:
        void* module;
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
