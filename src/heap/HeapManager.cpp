#include <iterator>
#include <mutex>
#include <typeinfo>
#include "HeapManager.h"
#include "heap.h"
#include "../CatLa.h"

using namespace heap;
using namespace cat_vm;
using namespace std;

HeapManager::HeapManager(size_t cells) {
    this->vm = virtual_machine;
    this->cells_size = cells;
    this->heap_info_cells = new HeapManagerCell*[cells];

    for (size_t i = 0; i < cells; i++) {
        heap_info_cells[i] = new HeapManagerCell(this);
    }
}

HeapManager::~HeapManager() {
    for (size_t i = 0; i < cells_size; i++) {
        delete heap_info_cells[i];
    }
    delete[] heap_info_cells;
}

bool HeapManager::is_released(size_t runtime_object_id) {
    return heap_info_cells[runtime_object_id % cells_size]->is_released(runtime_object_id);
}

void HeapManager:: release(TreeHeapObject* object, size_t runtime_object_id) {
    heap_info_cells[runtime_object_id % cells_size]->release(object, runtime_object_id);
}

bool HeapManager::collect_and_check_roots_to_release(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* roots_copy) {
    return heap_info_cells[runtime_object_id % cells_size]->collect_and_check_roots_to_release(object, runtime_object_id, roots_copy);
}

void HeapManager::try_to_release_fields(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* tree_fields) {
    heap_info_cells[runtime_object_id % cells_size]->try_to_release_fields(object, runtime_object_id, tree_fields);
}

void HeapManager::show_heap_info() {
    printf("======  Heap memory info  ======\n");
    bool all_released = true;
    for (size_t i = 1; i < static_runtime_object_id; i++) {
        bool is_released = heap_manager->is_released(i);
        if (!is_released) {
            all_released = false;
        }

        printf("%zu : %s\n", i, is_released ? "Released" : "Leaving");
    }
    printf("================================\n");

    if (all_released) {
        printf("Released all!\n");
    }
}


HeapManagerCell::HeapManagerCell(HeapManager* manager) {
    this->manager = manager;
}

bool HeapManagerCell::is_released(size_t runtime_object_id) {
    size_t index = runtime_object_id / manager->cells_size;
    size_t array_index = index / 8;
    size_t byte_index = index % 8;

    lock.lock();
    size_t length = released_ids.size();
    if (length <= array_index) {
        size_t add = (array_index - length) + 1;
        for (size_t i = 0; i < add; i++) {
            released_ids.push_back(0);
        }
    }

    uint8_t cell = released_ids[array_index];
    bool released = (cell & (1 << byte_index)) != 0;
    lock.unlock();

    return released;
}

bool HeapManagerCell::is_safe_release(TreeHeapObject* object, size_t runtime_object_id) {
    size_t index = runtime_object_id / manager->cells_size;
    size_t array_index = index / 8;
    size_t byte_index = index % 8;

    lock.lock();
    size_t length = released_ids.size();

    bool leaving = false;
    if (length <= array_index) {
        leaving = true;
        size_t add = (array_index - length) + 1;
        for (size_t i = 0; i < add; i++) {
            released_ids.push_back(0);
        }
    } else {
        uint8_t cell = released_ids[array_index];
        leaving = (cell & (1 << byte_index)) == 0;
    }

    if (!leaving) {
        lock.unlock();
        return false;
    }

    if (object->normal_reference_count != 0) {
        lock.unlock();
        return false;
    }

    lock.unlock();
    return true;
}

void HeapManagerCell::release(TreeHeapObject* object, size_t runtime_object_id) {
    size_t index = runtime_object_id / manager->cells_size;
    size_t array_index = index / 8;
    size_t byte_index = index % 8;

    lock.lock();
    size_t length = released_ids.size();

    bool leaving = false;
    uint8_t new_cell = 0;
    if (length <= array_index) {
        leaving = true;
        size_t add = (array_index - length) + 1;
        for (size_t i = 0; i < add; i++) {
            released_ids.push_back(0);
        }
    } else {
        uint8_t cell = released_ids[array_index];
        leaving = (cell & (1 << byte_index)) == 0;
        new_cell = cell;
    }

    if (leaving) {
        released_ids[array_index] = new_cell | (1 << byte_index);
        object->unsafe_release();
    }

    lock.unlock();
}

bool HeapManagerCell::collect_and_check_roots_to_release(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* roots_copy) {
    size_t index = runtime_object_id / manager->cells_size;
    size_t array_index = index / 8;
    size_t byte_index = index % 8;

    lock.lock();
    size_t length = released_ids.size();

    bool leaving = false;
    if (length <= array_index) {
        leaving = true;
        size_t add = (array_index - length) + 1;
        for (size_t i = 0; i < add; i++) {
            released_ids.push_back(0);
        }
    } else {
        uint8_t cell = released_ids[array_index];
        leaving = (cell & (1 << byte_index)) == 0;
    }

    if (!leaving) {
        lock.unlock();
        return true;
    }

    if (object->normal_reference_count != 0) {
        lock.unlock();
        return false;
    }

    object->lock.lock();
    for (auto it = object->held_roots.begin(); it != object->held_roots.end(); ++it) {
        HeapObjectIdPair* pair = (*it);
        roots_copy->push_back(pair->clone());
    }
    object->lock.unlock();

    lock.unlock();
    return true;
}

void HeapManagerCell::try_to_release_fields(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* tree_fields) {
    size_t index = runtime_object_id / manager->cells_size;
    size_t array_index = index / 8;
    size_t byte_index = index % 8;
    
    lock.lock();
    size_t length = released_ids.size();

    bool leaving = false;
    if (length <= array_index) {
        leaving = true;
        size_t add = (array_index - length) + 1;
        for (size_t i = 0; i < add; i++) {
            released_ids.push_back(0);
        }
    } else {
        uint8_t cell = released_ids[array_index];
        leaving = (cell & (1 << byte_index)) == 0;
    }

    if (!leaving) {
        lock.unlock();
        return;
    }

    if (object->normal_reference_count != 0) {
        lock.unlock();
        return;
    }

    for (size_t i = 0; i < object->field_capacity; i++) {
        HeapObjectIdPair* pair = object->fields[i];
        if (pair == nullptr) {
            continue;
        }

        if (pair->runtime_object_id == 0) {
            HeapObject* primitive_object = pair->object;
            primitive_object->unsafe_release();
        } else {
            tree_fields->push_back(pair->clone());
        }
    }

    lock.unlock();
}