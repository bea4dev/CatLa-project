#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <vm/parser/Structs.h>
#include <vm/PrimitiveType.h>

using namespace std;
using namespace type;

namespace modules {

    class Type {

    public:
        void* module;
        PrimitiveType* primitive_type;
        std::string type_name;
        size_t runtime_type_id;
        vector<FieldInfo> field_infos;

        TypeInfo parent_info;
        Type* parent;

        Type(PrimitiveType* primitive_type, std::string type_name, size_t runtime_type_id, const vector<FieldInfo>& field_infos, TypeInfo parent_info);

    };


}
