#pragma once

#include <stdint.h>
#include <stdio.h>
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


    class ReferenceCounterObject : public HeapObject {
    public:
        size_t runtime_object_id;
        atomic_size_t normal_reference_count = 0;

        ReferenceCounterObject();
    };



    class eapObject;
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


    class TreeHeapObject : public ReferenceCounterObject {

    public:
        size_t field_capacity;
        HeapObjectIdPair** fields = nullptr;
        mutex lock;
        vector<HeapObjectIdPair*> held_roots;


    public:
        TreeHeapObject(size_t field_capacity);
        ~TreeHeapObject();

        void hold();

        void drop();

        void set_field_object(HeapObject* object, size_t object_id, size_t field_index);

        HeapObject* get_field_object(size_t field_index);

        void unsafe_release() override;

        void add_root_object(TreeHeapObject* root);

    };



    bool collect_and_check_to_release(TreeHeapObject* object, size_t current_object_id, unordered_map<size_t, TreeHeapObject*>* roots_map);

    void safe_release(TreeHeapObject* object, size_t runtime_object_id);

}
