#pragma once
#include "CatLaClass.h"
#include "CatLaFunction.h"
#include "modules.h"

namespace modules {

    class CatLaModule {
    public:

        size_t fields_length;
        CatLaFunction** functions;
    };

}