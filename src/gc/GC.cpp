#include <gc/GC.h>
#include <unordered_map>
#include <unordered_set>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new vector<HeapObject*>;
    this->collector_lock = PTHREAD_MUTEX_INITIALIZER;
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

    unordered_set<HeapObject*> release_objects;

    //Collect all suspected objets
    for (auto& object_ptr : *suspected_object_list_old) {
        size_t object_ptr_flag = object_ptr->flag.load(std::memory_order_acquire);
        if (object_ptr_flag == 3 || object_ptr_flag == 5) {
            atomic_thread_fence(std::memory_order_acquire);
            release_objects.insert(object_ptr);
            continue;
        }

        auto* current_object = object_ptr;

        this->collect_lock.write_lock();

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
        object_reference_count_map[object_ptr] = object_ptr->count.load(std::memory_order_acquire);

        //Collect and mark gray.
        while (true) {
            //size_t object_flag = current_object->flag.load(std::memory_order_acquire);

            auto* current_object_type = (Type*) current_object->type_info;
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                if (get_flag(current_object_type->reference_fields, i)) {
                    auto* field_object = fields[i];
                    if (field_object != nullptr) {
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
                            collecting_objects.push_back(field_object);
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
        //Reset current object.
        current_object = object_ptr;

        //Mark white
        object_ptr->flag.store(5, std::memory_order_release);

        //Mark white or black.
        while (true) {
            size_t object_flag = current_object->flag.load(std::memory_order_acquire);

            size_t current_object_reference_count = object_reference_count_map[current_object];
            //Mark black.
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
                    if (field_object != nullptr) {
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
            }

            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }

        printf("COLLECT!\n");
        //Release white.
        for (auto& object : collecting_objects) {
            size_t object_flag = object->flag.load(std::memory_order_acquire);
            if (object_flag == 5) {
                //If object is white
                //Release
                printf("COLLECT WHITE! [%p] : %p %p\n", object, *((HeapObject**) (object + 1)), *((HeapObject**) (object + 1) + 1));
                //object->flag.store(3, std::memory_order_release);
                //atomic_thread_fence(std::memory_order_acquire);
                release_objects.insert(object);
            } else {
                //printf("FLAG [%p] : %llu : %llu\n", object, object_flag, object->count.load(std::memory_order_acquire));
                object->flag.store(1, std::memory_order_release);
            }
        }

        for (auto& object : collecting_objects) {
            object_unlock(object);
        }

        this->collect_lock.write_unlock();
    }

    for (auto& object : release_objects) {
        auto* object_type = (Type*) object->type_info;
        size_t field_length = object->field_length;
        auto** fields = (HeapObject**) (object + 1);
        auto* reference_fields = object_type->reference_fields;
        size_t object_flag = object->flag.load(std::memory_order_acquire);

        for (size_t i = 0; i < field_length; i++) {
            if (get_flag(reference_fields, i)) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    auto* field_object_type = (Type*) field_object->type_info;
                    bool is_cycle_type = field_object_type->is_cycling_type.load(std::memory_order_acquire);
                    size_t field_object_flag = field_object->flag.load(std::memory_order_acquire);
                    if (is_cycle_type) {
                        if (object_flag == 5) {
                            if (field_object_flag == 1 || field_object_flag == 2) {
                                decrease_reference_count(this, field_object);
                            }
                        }
                    } else {
                        decrease_reference_count(this, field_object);
                    }
                }
            }
        }

        //Release
        //object->count.store(0, std::memory_order_release);
        object->flag.store(0, std::memory_order_release);
    }

    delete suspected_object_list_old;

    pthread_mutex_unlock(&collector_lock);
}