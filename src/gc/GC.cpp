#include <gc/GC.h>
#include <unordered_map>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new vector<HeapObject*>;
}

static size_t block_size_list[13]{32, 40, 48, 56, 64, 72, 80, 88, 96, 128, 256, 384, 512};

void CycleCollector::collect_cycles() {
    //Locking to limit Cycle Collector to one thread.
    pthread_mutex_lock(&collector_lock);

    //Move list ptr
    list_lock.lock();
    auto* suspected_object_list_old = this->suspected_object_list;
    this->suspected_object_list = new vector<HeapObject*>;
    list_lock.unlock();

    //Collect all suspected objets
    for (auto& object_ptr : *suspected_object_list_old) {
        auto* current_object = object_ptr;

        object_lock(object_ptr);

        vector<HeapObject*> collecting_objects;
        collecting_objects.push_back(object_ptr);
        //Mark gray.
        //--- flag ---
        //4 : gray
        //5 : white
        //6 : black
        object_ptr->flag.store(4, std::memory_order_release);

        stack<HeapObject*> check_objects;
        unordered_map<HeapObject*, size_t> object_reference_count_map;

        this->collect_lock.write_lock();

        //Collect and mark gray.
        while (true) {
            size_t object_flag = current_object->flag.load(std::memory_order_acquire);
            if (object_flag == 3) {
                break;
            }

            auto* current_object_type = (Type*) current_object->type_info;
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                if (get_flag(current_object_type->reference_fields, i)) {
                    auto* field_object = fields[i];
                    if (field_object->flag.load(std::memory_order_acquire) == 4) {
                        //Is gray.
                        size_t field_object_reference_count = object_reference_count_map[field_object];
                        object_reference_count_map[field_object] = field_object_reference_count - 1;
                    } else {
                        //Mark gray.
                        object_lock(field_object);
                        field_object->flag.store(4, std::memory_order_release);
                        size_t field_object_reference_count = field_object->count.load(std::memory_order_acquire);
                        object_reference_count_map[field_object] = field_object_reference_count - 1;
                        check_objects.push(field_object);
                    }
                }
            }

            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }
        //Reset current object.
        current_object = object_ptr;

        //Mark white
        object_ptr->flag.store(5, std::memory_order_release);

        //Mark white or black.
        while (true) {
            size_t object_flag = current_object->flag.load(std::memory_order_acquire);
            if (object_flag == 3) {
                break;
            }

            size_t current_object_reference_count = object_reference_count_map[current_object];
            if (current_object_reference_count > 0) {
                object_flag = 6;
                current_object->flag.store(6, std::memory_order_release);
            }

            auto* current_object_type = (Type*) current_object->type_info;
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                if (get_flag(current_object_type->reference_fields, i)) {
                    auto* field_object = fields[i];
                    size_t field_object_flag = field_object->flag.load(std::memory_order_acquire);

                    if (object_flag == 6) {
                        //If current object is black.
                        //Increase reference count for field object.
                        size_t field_object_reference_count = object_reference_count_map[field_object];
                        object_reference_count_map[field_object] = field_object_reference_count + 1;
                    }

                    if (field_object_flag < object_flag) {
                        field_object_flag = object_flag;
                        field_object->flag.store(field_object_flag, std::memory_order_release);
                        check_objects.push(field_object);
                    }
                }
            }

            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }

        //Release white.
        for (auto& object : collecting_objects) {
            if (object->flag.load(std::memory_order_acquire) == 5) {
                //If object is white.
                //Release.
                object->flag.store(0, std::memory_order_release);
            } else {
                object->flag.store(1, std::memory_order_release);
            }
            object_unlock(object);
        }

        this->collect_lock.write_unlock();
    }

    delete suspected_object_list_old;

    pthread_mutex_unlock(&collector_lock);
}