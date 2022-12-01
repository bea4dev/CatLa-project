#include <gc/GC.h>
#include <unordered_map>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->collector_lock = PTHREAD_MUTEX_INITIALIZER;
}

enum object_color : uint8_t {
    red,
    gray,
    white,
    black
};

void CycleCollector::gc_collect() {
    //Locking to limit Cycle Collector to one thread.
    pthread_mutex_lock(&collector_lock);

    //Move list
    this->list_lock.lock();
    auto* roots = this->suspected_object_list;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->list_lock.unlock();

    unordered_set<HeapObject*> release_objects;

    for (auto& root : *roots) {
        stack<HeapObject*> check_objects;
        unordered_map<HeapObject*, uint8_t> color_map;
        unordered_map<HeapObject*, size_t> count_map;
        vector<HeapObject*> cycle_objects;
        auto* current_object = root;
        bool is_cyclic_root = false;

        //Mark red (Lock phase)
        while (true) {
            //Lock
            color_map[current_object] = object_color::red;
            object_lock(current_object);
            cycle_objects.push_back(current_object);

            //Get field objects
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    if (color_map.find(field_object) == color_map.end()) {
                        check_objects.push(field_object);
                    } else {
                        if (field_object == root) {
                            is_cyclic_root = true;
                        }
                    }
                }
            }

            //Update current object
            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }


        if (is_cyclic_root) {
            //Mark gray
            count_map[root] = root->count.load(std::memory_order_acquire);
            current_object = root;
            while (true) {
                color_map[current_object] = object_color::gray;

                //Get field objects
                auto** fields = (HeapObject**) (current_object + 1);
                size_t field_length = current_object->field_length;
                for (size_t i = 0; i < field_length; i++) {
                    auto* field_object = fields[i];
                    if (field_object != nullptr) {
                        if (color_map[field_object] == object_color::gray) {
                            size_t previous_rc = count_map[field_object];
                            if (previous_rc > 0) {
                                count_map[field_object] = previous_rc - 1;
                            } else {
                                printf("1 : ?????\n");
                            }
                        } else {
                            check_objects.push(field_object);
                            count_map[field_object] = field_object->count.load(std::memory_order_acquire) - 1;
                        }
                    }
                }

                //Update current object
                if (check_objects.empty()) {
                    break;
                }
                current_object = check_objects.top();
                check_objects.pop();
            }

            //Mark white or black
            current_object = root;
            if (count_map[root] == 0) {
                color_map[root] = object_color::white;
            } else {
                color_map[root] = object_color::black;
            }
            while (true) {
                uint8_t current_object_color = color_map[current_object];

                //Get field objects
                auto** fields = (HeapObject**) (current_object + 1);
                size_t field_length = current_object->field_length;
                for (size_t i = 0; i < field_length; i++) {
                    auto* field_object = fields[i];
                    if (field_object != nullptr) {
                        uint8_t field_object_color = color_map[field_object];
                        size_t field_object_rc = count_map[field_object];
                        if (current_object_color == object_color::white) {
                            if (field_object_rc != 0) {
                                color_map[field_object] = object_color::black;
                            } else if (field_object_color == object_color::gray) {
                                color_map[field_object] = object_color::white;
                                check_objects.push(field_object);
                            }
                        } else if (current_object_color == object_color::black) {
                            count_map[field_object] = field_object_rc + 1;
                            if (field_object_color != object_color::black) {
                                color_map[field_object] = object_color::black;
                                check_objects.push(field_object);
                            }
                        }
                    }
                }

                if (check_objects.empty()) {
                    break;
                }
                current_object = check_objects.top();
                check_objects.pop();
            }

            //Collect white
            for (auto& object : cycle_objects) {
                if (color_map[object] == object_color::white) {
                    object->state.store(object_state::waiting_for_gc);
                    release_objects.insert(object);
                }
            }

            for (auto& object : cycle_objects) {
                object_unlock(object);
            }
        } else {
            for (auto& object : cycle_objects) {
                object_unlock(object);
            }

            unordered_set<HeapObject*> non_cycle_objects;
            current_object = root;
            bool ready_to_release = true;
            while (true) {
                if (current_object->state.load(std::memory_order_acquire) != object_state::waiting_for_gc) {
                    ready_to_release = false;
                    break;
                }
                non_cycle_objects.insert(current_object);

                auto** fields = (HeapObject**) (current_object + 1);
                size_t field_length = current_object->field_length;
                for (size_t i = 0; i < field_length; i++) {
                    auto* field_object = fields[i];
                    if (field_object != nullptr && non_cycle_objects.find(field_object) == non_cycle_objects.end()) {
                        check_objects.push(field_object);
                    }
                }

                if (check_objects.empty()) {
                    break;
                }
                current_object = check_objects.top();
                check_objects.pop();
            }

            if (ready_to_release) {
                for (auto& object : non_cycle_objects) {
                    release_objects.insert(object);
                }
            }
        }
    }

    for (auto& object : release_objects) {
        roots->erase(object);
        if (object->is_cyclic_type && object->async_release.load(std::memory_order_acquire)) {
            this->list_lock.lock();
            this->suspected_object_list->erase(object);
            this->list_lock.unlock();
        }

        auto** fields = (HeapObject**) (object + 1);
        size_t field_length = object->field_length;
        for (size_t i = 0; i < field_length; i++) {
            auto* field_object = fields[i];
            if (field_object != nullptr) {
                if (field_object->state.load(std::memory_order_acquire) == object_state::live) {
                    decrement_reference_count(this, field_object);
                }
            }
        }
    }

    for (auto& object : release_objects) {
        object->state.store(object_state::dead, std::memory_order_release);
    }

    for (auto& root : *roots) {
        this->list_lock.lock();
        this->suspected_object_list->insert(root);
        this->list_lock.unlock();
    }

    delete roots;

    pthread_mutex_unlock(&collector_lock);
}
