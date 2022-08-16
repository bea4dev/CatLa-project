#pragma once
#include <vm/NyanVM.h>

using namespace nyan;

namespace stack {

    extern size_t get_available_stack_size(VMThread* vm_thread);

}