#pragma once

#include <vector>
#include <mutex>
#include "heap.h"
#include "HeapObject.h"

namespace cat_vm {
    class CatVM;
}

namespace heap {
    class ReferenceCounterObject;
    class TreeHeapObject;
    class HeapObjectIdPair;
}

using namespace std;
using namespace cat_vm;

namespace heap {
    
    class HeapManagerCell;



    class HeapManager {

    private:
        HeapManagerCell** heap_info_cells;

    public:
        CatVM* vm;
        size_t cells_size;

        HeapManager(size_t cells);
        ~HeapManager();
        
        bool is_released(size_t runtime_object_id);

        void release(TreeHeapObject* object, size_t runtime_object_id);

        bool collect_and_check_roots_to_release(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* roots_copy);

        void try_to_release_feilds(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* tree_fields);

        void show_heap_info();
    };



    class HeapManagerCell {

    private:
        HeapManager* manager;
        vector<uint8_t> released_ids;
        mutex lock;
    
    public:
        HeapManagerCell(HeapManager* manager);

        bool is_released(size_t runtime_object_id);

        bool is_safe_release(TreeHeapObject* object, size_t runtime_object_id);

        void release(TreeHeapObject* object, size_t runtime_object_id);

        bool collect_and_check_roots_to_release(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* roots_copy);

        void try_to_release_feilds(TreeHeapObject* object, size_t runtime_object_id, vector<HeapObjectIdPair*>* tree_fields);
    };

}
