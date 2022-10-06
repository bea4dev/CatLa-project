#pragma once
#include <vm/CatVM.h>

using namespace catla;

namespace stack {

    extern size_t get_available_stack_size(VMThread* vm_thread);

}