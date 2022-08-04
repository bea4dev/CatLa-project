#pragma once

#include "CodeBlock.h"

namespace modules {

    class CatLaFunction {
    public:
        size_t arg_length;
        bool has_return;
        CodeBlock* code_block;

    public:
        CatLaFunction(size_t arg_length, bool has_return, CodeBlock* code_block);
        ~CatLaFunction();
    };

}