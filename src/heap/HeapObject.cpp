#pragma once

#include "HeapObject.h"
#include <iterator>
#include <unordered_set>

using namespace std;
using namespace heap;
using namespace modules;


atomic_size_t static_runtime_object_id;

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
    for (auto it = held_roots.begin(); it != held_roots.end(); ++it) {
        HeapObjectIdPair* pair = (*it);
        delete pair;
    }
    delete[] fields;
}

void TreeHeapObject::hold() {
    printf("HOLD : %zu\n", runtime_object_id);
    normal_reference_count.fetch_add(1);
}

void TreeHeapObject::drop() {
    printf("DROP : %zu\n", runtime_object_id);
    size_t object_id = runtime_object_id;
    size_t before = normal_reference_count.fetch_sub(1);

    if (before == 1) {
        heap::safe_release(this, object_id);
    }
}

void TreeHeapObject::set_field_object(HeapObject* object, size_t object_id, size_t field_index) {
    lock.lock();
    HeapObjectIdPair* pair = fields[field_index];
    delete pair;

    fields[field_index] = new HeapObjectIdPair(object, object_id);
    lock.unlock();

    if (object_id != 0) {
        auto* tree_object = (TreeHeapObject*) object;
        tree_object->add_root_object(this);
    }
}

HeapObject* TreeHeapObject::get_field_object(size_t field_index) {
    lock.lock();
    HeapObject* object = fields[field_index]->object;
    lock.unlock();
    return object;
}

void TreeHeapObject::unsafe_release() {
    printf("TreeObject Released! %zu\n", runtime_object_id);
    delete this;
}


void TreeHeapObject::add_root_object(TreeHeapObject* root) {
    lock.lock();
    held_roots.push_back(new HeapObjectIdPair(root, root->runtime_object_id));
    lock.unlock();
}




bool heap::collect_and_check_to_release(TreeHeapObject* object, size_t object_id, unordered_map<size_t, TreeHeapObject*>* roots_map) {
    //roots_mapはフィールド取得済み、reserved_objectsは到達済み
    unordered_map<size_t, TreeHeapObject*> reserved_object_map;

    TreeHeapObject* current_object = object;
    size_t current_object_id = object_id;
    while (true) {
        vector<HeapObjectIdPair*> held_copy;
        bool safe = heap_manager->collect_and_check_roots_to_release(current_object, current_object_id, &held_copy);
        if (!safe) {
            return false;
        }
        (*roots_map)[current_object_id] = current_object;


        for (auto it = held_copy.begin(); it != held_copy.end(); ++it) {
            HeapObjectIdPair* pair = (*it);
            size_t root_object_id = pair->runtime_object_id;

            if (roots_map->count(root_object_id) != 0) {
                continue;
            }
            if (reserved_object_map.count(root_object_id) != 0) {
                continue;
            }

            auto* root_object = (TreeHeapObject*) pair->object;
            reserved_object_map[root_object_id] = root_object;

            delete pair;
        }


        if (reserved_object_map.empty()) {
            return true;
        }

        //pop
        auto first = reserved_object_map.begin();
        current_object_id = first->first;
        current_object = first->second;
        reserved_object_map.erase(current_object_id);
    }
}


void heap::safe_release(TreeHeapObject* object, size_t runtime_object_id) {
    unordered_set<size_t> released_object_id_set;
    unordered_map<size_t, TreeHeapObject*> reserved_object_map;
    TreeHeapObject* current_object = object;
    size_t current_object_id = runtime_object_id;

    while (true) {
        unordered_map<size_t, TreeHeapObject*> garbage_object_map;
        bool safe = heap::collect_and_check_to_release(current_object, current_object_id, &garbage_object_map);
        if (!safe) {
            if (reserved_object_map.empty()) {
                return;
            }
            continue;
        }

        vector<HeapObjectIdPair*> tree_fields;
        heap_manager->try_to_release_fields(current_object, current_object_id, &tree_fields);

        heap_manager->release(current_object, current_object_id);
        reserved_object_map.erase(current_object_id);
        released_object_id_set.insert(current_object_id);

        for (auto it = garbage_object_map.begin(); it != garbage_object_map.end(); ++it) {
            size_t object_id_ = it->first;
            TreeHeapObject* object_ = it->second;

            if (released_object_id_set.count(object_id_) != 0) {
                continue;
            }
            if (reserved_object_map.count(object_id_) != 0) {
                continue;
            }

            reserved_object_map[object_id_] = object_;
        }

        for (auto it = tree_fields.begin(); it != tree_fields.end(); ++it) {
            HeapObjectIdPair* tree_field_pair = (*it);
            size_t tree_field_object_id = tree_field_pair->runtime_object_id;

            if (released_object_id_set.count(tree_field_object_id) != 0) {
                continue;
            }
            if (reserved_object_map.count(tree_field_object_id) != 0) {
                continue;
            }

            auto* tree_field_object = (TreeHeapObject*) tree_field_pair->object;
            reserved_object_map[tree_field_object_id] = tree_field_object;

            delete tree_field_pair;
        }

        if (reserved_object_map.empty()) {
            return;
        }

        //pop
        auto first = reserved_object_map.begin();
        current_object_id = first->first;
        current_object = first->second;
        reserved_object_map.erase(current_object_id);
    }
}