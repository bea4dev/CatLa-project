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

};

class Function {
public:
    string name;
    vector<LabelBlock*> label_blocks;
    unordered_map<string, LabelBlock*> label_block_map;

};

