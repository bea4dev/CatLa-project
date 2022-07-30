#pragma once

#include "HeapObject.h"
#include <iterator>

using namespace std;
using namespace heap;
using namespace modules;


atomic_size_t static_runtime_object_id = 1;

size_t heap::new_runtime_object_id() {
    return static_runtime_object_id.fetch_add(1);
}



ReferenceCounterObject::ReferenceCounterObject() {
    this->runtime_object_id = heap::new_runtime_object_id();
}


TreeHeapObject::TreeHeapObject(size_t field_capacity) {
    this->field_capacity = field_capacity;
    this->fields = new HeapObjectIdPair*[field_capacity];

    for (size_t i = 0; i < field_capacity; i++) {
        this->fields[i] = nullptr;
    }
}

TreeHeapObject::~TreeHeapObject() {
    for (size_t i = 0; i < field_capacity; i++) {
        delete fields[i];
    }
    delete[] fields;
}

void TreeHeapObject::hold() {
    printf("HOLD : %d\n", runtime_object_id);
    normal_reference_count.fetch_add(1);
}

void TreeHeapObject::drop() {
    printf("DROP : %d\n", runtime_object_id);
    size_t object_id = runtime_object_id;
    size_t before = normal_reference_count.fetch_sub(1);

    if (before == 1) {
        heap::safe_release(this, object_id);
    }
}

void TreeHeapObject::set_field_object(HeapObject* object, size_t object_id, size_t field_index) {
    lock.lock();
    HeapObjectIdPair* pair = fields[field_index];
    if (pair != nullptr) {
        delete pair;
    }

    fields[field_index] = new HeapObjectIdPair(object, object_id);
    lock.unlock();

    if (object_id != 0) {
        TreeHeapObject* tree_object = (TreeHeapObject*) object;
        tree_object->add_root_object(this);
    }
}

HeapObject* TreeHeapObject::get_field_object(size_t field_index) {
    lock.lock();
    return fields[field_index]->object;
    lock.unlock();
}

void TreeHeapObject::unsafe_release() {
    printf("TreeObject Released! %d\n", runtime_object_id);
    delete this;
}


void TreeHeapObject::add_root_object(TreeHeapObject* root) {
    lock.lock();
    held_roots.push_back(new HeapObjectIdPair(root, root->runtime_object_id));
    lock.unlock();
}




bool heap::collect_and_check_to_release(TreeHeapObject* object, size_t current_object_id, unordered_map<size_t, TreeHeapObject*>* roots_map) {    
    vector<HeapObjectIdPair*> held_copy;
    bool safe = heap_manager->collect_and_check_roots_to_release(object, current_object_id, &held_copy);
    if (!safe) {
        return false;
    }

    (*roots_map)[current_object_id] = object;

    for (auto it = held_copy.begin(); it != held_copy.end(); ++it) {
        size_t root_object_id = (*it)->runtime_object_id;

        if (roots_map->count(root_object_id) != 0) {
            continue;
        }
        
        TreeHeapObject* root_object = (TreeHeapObject*) (*it)->object;
        bool safe = heap::collect_and_check_to_release(root_object, root_object_id, roots_map);
        if (!safe) {
            return false;
        }
    }

    return true;
}


void heap::safe_release(TreeHeapObject* object, size_t runtime_object_id) {
    unordered_map<size_t, TreeHeapObject*> garbage_object_map;
    bool safe = heap::collect_and_check_to_release(object, runtime_object_id, &garbage_object_map);
    if (!safe) {
        return;
    }

    vector<HeapObjectIdPair*> tree_fields;
    heap_manager->try_to_release_feilds(object, runtime_object_id, &tree_fields);

    for (auto it = garbage_object_map.begin(); it != garbage_object_map.end(); ++it) {
        size_t object_id = it->first;
        TreeHeapObject* object = it->second;
        heap_manager->release(object, object_id);
    }

    for (auto it = tree_fields.begin(); it != tree_fields.end(); ++it) {
        HeapObjectIdPair* tree_field_pair = (*it);
        heap::safe_release((TreeHeapObject*) tree_field_pair->object, tree_field_pair->runtime_object_id);
        delete tree_field_pair;
    }
}