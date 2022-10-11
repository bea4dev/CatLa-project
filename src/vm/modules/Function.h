#pragma once

#include <vm/modules/orders/Orders.h>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;


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
    PrimitiveType* return_type;
    vector<PrimitiveType*> argument_types;

public:
    Function(string name, size_t variables_size, size_t registers_size, vector<LabelBlock*> label_blocks, PrimitiveType* return_type, vector<PrimitiveType*> argument_types);

};

