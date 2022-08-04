#include "CatLaFunction.h"

using namespace modules;

CatLaFunction::CatLaFunction(size_t arg_length, bool has_return, CodeBlock* code_block) {
    this->arg_length = arg_length;
    this->has_return = has_return;
    this->code_block = code_block;
}

CatLaFunction::~CatLaFunction() {
    delete this->code_block;
}