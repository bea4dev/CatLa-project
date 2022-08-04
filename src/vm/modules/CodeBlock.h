#pragma once
#include <cstdint>
#include <vector>

using namespace std;

namespace modules {

    class CodeBlock {

    public:
        vector<uint8_t>* byte_code;
        vector<uint64_t>* const_values;

        CodeBlock(vector<uint8_t>* byte_code, vector<uint64_t>* const_values);

        ~CodeBlock();

    };

}