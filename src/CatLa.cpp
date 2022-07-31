#include <iostream>
#include "CatLa.h"
#include "heap/HeapManager.h"
#include <random>
#include <thread>

using namespace modules;

CatVM* virtual_machine = nullptr;
HeapManager* heap_manager = nullptr;

void setup_virtual_machine() {
    static_runtime_object_id.fetch_add(1);
    virtual_machine = new CatVM();
    heap_manager = new heap::HeapManager((size_t) 8);
}


int random() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 10000);
    return dist(gen);
}

void release(TreeHeapObject* object) {
    object->hold();
    object->drop();
}

int main()
{
    std::cout << "Hello World!\n";

    setup_virtual_machine();

    vector<TreeHeapObject*> objects;

    for (size_t i = 0; i < 10000; i++) {
        int r = random();
        int objects_count = objects.size();
        auto* object1 = new TreeHeapObject(3);
        auto* object2 = new TreeHeapObject(3);
        int index = 0;

        if (objects_count >= 1) {
            index = r % objects_count;
        }
        TreeHeapObject* random_object;
        if (objects_count == 0) {
            random_object = new TreeHeapObject(3);
            objects.push_back(random_object);
        } else {
            random_object = objects.at(index);
        }

        random_object->set_field_object(object1, object1->runtime_object_id, 0);
        random_object->set_field_object(object2, object2->runtime_object_id, 1);

        if (r % 8 == 0) {
            random_object->set_field_object(random_object, random_object->runtime_object_id, 2);
        } else if (r % 6 == 0) {
            random_object->set_field_object(object1, object1->runtime_object_id, 2);
        } else if (r % 2 == 0) {
            random_object->set_field_object(object2, object2->runtime_object_id, 2);
        } else {
            object1->set_field_object(random_object, random_object->runtime_object_id, 2);
            object2->set_field_object(random_object, random_object->runtime_object_id, 2);
        }

        auto remove_element = objects.begin() + index;
        objects.erase(remove_element);
        objects.push_back(object1);
        objects.push_back(object2);
    }

    int r = random();
    int objects_count = objects.size();
    int index = r % objects_count;

    auto* random_object1 = objects.at(index);

    r = random();
    index = r % objects_count;

    auto* random_object2 = objects.at(index);

    r = random();
    index = r % objects_count;

    auto* random_object3 = objects.at(index);

    r = random();
    index = r % objects_count;

    auto* random_object4 = objects.at(index);


    heap_manager->show_heap_info();

    thread thread1(release, random_object1);
    thread thread2(release, random_object2);
    thread thread3(release, random_object3);
    thread thread4(release, random_object4);

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    heap_manager->show_heap_info();

    std::cout << "Complete!\n";
}