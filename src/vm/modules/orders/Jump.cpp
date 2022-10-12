#include <vm/modules/orders/Jump.h>
#include <utility>
#include <vm/modules/Function.h>
#include <vm/CatVM.h>

using namespace catla;

JumpToLabel::JumpToLabel(string label_name) {
    this->label_name = std::move(label_name);
    this->label = nullptr;
}

void JumpToLabel::eval(void* vm_thread, void* module, uint64_t* registers, uint64_t* variables, uint64_t* arguments) {
    auto* label_block = (LabelBlock*) this->label;
    auto* thread = (VMThread*) vm_thread;
    thread->current_label_block = label_block;
    thread->current_order_index = 0;
}
