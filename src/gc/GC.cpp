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

    for (auto& root : *roots) {
        stack<HeapObject*> check_objects;
        unordered_map<HeapObject*, uint8_t> color_map;
        unordered_map<HeapObject*, size_t> count_map;
        vector<HeapObject*> cycle_objects;
        auto* current_object = root;
        bool is_cyclic = false;

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
                        is_cyclic = true;
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
    }



    this->list_lock.lock();
    for (auto& root : *roots) {
        this->suspected_object_list->insert(root);
    }
    this->list_lock.unlock();

    delete roots;

    pthread_mutex_unlock(&collector_lock);
}
