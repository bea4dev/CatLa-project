#pragma once
#include "CatLaClass.h"
#include "CatLaFunction.h"
#include "modules.h"

using namespace heap;

namespace modules {

    class CatLaModule {
    public:
        HeapObject** module_fields;
        size_t fields_length;
        CatLaFunction** functions;
    };

}