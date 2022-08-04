#include "CodeBlock.h"

using namespace modules;

CodeBlock::CodeBlock(vector<uint8_t>* byte_code, vector<uint64_t>* const_values) {
    this->byte_code = byte_code;
    this->const_values = const_values;
}

CodeBlock::~CodeBlock() {
    delete this->byte_code;
    delete this->const_values;
}