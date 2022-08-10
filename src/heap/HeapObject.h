#pragma once

#include <cstdint>
#include <cstdio>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "heap.h"

using namespace std;
using namespace modules;

extern atomic_size_t static_runtime_object_id;

namespace heap {

    size_t new_runtime_object_id();


    class HeapObject {

    public:
        size_t runtime_object_id;
        virtual void unsafe_release() = 0;
        
    };


    template<typename T> class PrimitiveHeapObject : public HeapObject {

    private:
        T value;

    public:
        PrimitiveHeapObject(T value) {
            this->value = value;
        }

        void unsafe_release() {
            delete this;
        }

        PrimitiveHeapObject* copy() {
            return new PrimitiveHeapObject(value);
        }

    };



    class HeapObjectIdPair {
    public:
        HeapObject* object;
        size_t runtime_object_id;

        HeapObjectIdPair(HeapObject* object, size_t runtime_object_id) {
            this->object = object;
            this->runtime_object_id = runtime_object_id;
        }

        HeapObjectIdPair* clone() {
            return new HeapObjectIdPair(object, runtime_object_id);
        }
    };


    class TreeHeapObject : public HeapObject {

    public:
        CatLaClass* class_info;
        atomic_size_t normal_reference_count;
        size_t local_thread_reference_count;
        bool is_arc;
        size_t local_thread;
        size_t field_capacity;
        HeapObject** fields = nullptr;
        size_t* field_ids = nullptr;
        mutex lock;
        unordered_map<size_t, TreeHeapObject*> local_thread_root_id_map;
        unordered_map<size_t, TreeHeapObject*> global_root_id_map;


    public:
        TreeHeapObject(CatLaClass* class_info, bool is_arc, size_t local_thread, size_t field_capacity);
        ~TreeHeapObject();

        void hold(size_t thread_id);

        void drop(size_t thread_id);

        void set_field_object(HeapObject* object, size_t object_id, size_t field_index, size_t thread_id);

        HeapObject* get_field_object(size_t field_index) const;

        void unsafe_release() override;

        void add_root_object(TreeHeapObject* root, size_t thread_id);

    };



    bool collect_and_check_to_release(TreeHeapObject* object, size_t current_object_id, unordered_map<size_t, TreeHeapObject*>* roots_map);

    void safe_release(TreeHeapObject* object, size_t runtime_object_id);

}
