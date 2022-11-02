#include <gc/GC.h>
#include <vm/CatVM.h>

using namespace gc;
using namespace catla;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
}

static size_t block_size_list[13]{32, 40, 48, 56, 64, 72, 80, 88, 96, 128, 256, 384, 512};

void CycleCollector::collect_cycles() {
    auto* virtual_machine = (CatVM*) this->vm;
    auto chunk_list = virtual_machine->clone_heap_chunk_list();

    for (auto &chunk: chunk_list) {
        auto *block_info_list = chunk->block_info_list;
        size_t cells_size = chunk->cells_size;

        for (size_t i = 0; i < 13; i++) {
            auto block_info = block_info_list[i];
            size_t block_size = block_size_list[i];
            uint8_t *entry_position = block_info.entry_position;

            for (size_t cell_index = 0; cell_index < cells_size; cell_index++) {
                auto *object = (HeapObject *) (entry_position + (block_size * cell_index));
                size_t flag = object->flag.load(std::memory_order_acquire);
                if (flag == 2) {

                }
            }
        }
    }
}