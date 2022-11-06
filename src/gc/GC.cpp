#include <gc/GC.h>
#include <unordered_map>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new vector<HeapObject*>;
}

static size_t block_size_list[13]{32, 40, 48, 56, 64, 72, 80, 88, 96, 128, 256, 384, 512};

void CycleCollector::collect_cycles() {
    //Move list ptr
    list_lock.lock();
    auto* suspected_object_list_old = this->suspected_object_list;
    this->suspected_object_list = new vector<HeapObject *>;
    list_lock.unlock();

    //Collect all suspected objets
    for (auto& object_ptr : *suspected_object_list_old) {
        auto* current_object = object_ptr;
        vector<HeapObject*> collecting_objects;
        stack<HeapObject*> object_temp_stack;
        unordered_map<HeapObject*, size_t> object_reference_count_map;
        while (true) {
            size_t expected = 2;
            if (!current_object->flag.compare_exchange_strong(expected, 4, std::memory_order_acquire)) {
                continue;
            }

            object_lock(current_object);
            collecting_objects.push_back(current_object);

            size_t field_length = current_object->field_length;
            auto** field_objects = (HeapObject**) (current_object + 1);
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = field_objects[i];
                if (field_object->flag.load(std::memory_order_relaxed) == 4) {
                    continue;
                }

                size_t count = field_object->count.load(std::memory_order_relaxed);
                if (count == 0) {
                    //Release
                    field_object->flag.store(0, std::memory_order_relaxed);
                    continue;
                }

                object_temp_stack.push(field_object);
                object_reference_count_map[field_object] = count - 1;
            }
        }
    }

    delete suspected_object_list_old;
}