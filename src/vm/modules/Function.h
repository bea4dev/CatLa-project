#pragma once

#include <vm/modules/orders/Orders.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <vm/parser/Structs.h>
#include <vm/modules/Type.h>

using namespace std;
using namespace modules;

class LabelBlock {
public:
    string name;
    vector<Order*> orders;

public:
    LabelBlock(string name, vector<Order*> orders);

};

class Function {
public:
    string name;
    size_t variables_size;
    size_t registers_size;
    vector<LabelBlock*> label_blocks;
    ArgumentInfo return_type_info;
    Type* return_type;
    vector<ArgumentInfo> argument_type_infos;
    vector<Type*> argument_types;

public:
    Function(string name, size_t variables_size, size_t registers_size, vector<LabelBlock*> label_blocks, const ArgumentInfo& return_type_info, const vector<ArgumentInfo>& argument_type_infos);

};

