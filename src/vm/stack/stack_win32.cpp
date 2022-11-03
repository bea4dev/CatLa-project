#include "StackUtil.h"
//#include <windows.h>

size_t util::get_available_stack_size(VMThread* vm_thread) {
    uint8_t a = 0;
    return vm_thread->stack_size - (vm_thread->top_of_stack_address - (size_t) &a);
}
