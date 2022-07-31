#include <iostream>
#include "CatLa.h"
#include "heap/HeapManager.h"
#include <random>

using namespace modules;

CatVM* virtual_machine = nullptr;
HeapManager* heap_manager = nullptr;

void setup_virtual_machine() {
    static_runtime_object_id.fetch_add(1);
    virtual_machine = new CatVM();
    heap_manager = new heap::HeapManager((size_t) 8);
}

int main()
{
    std::cout << "Hello World!\n";

    setup_virtual_machine();


    auto* object1 = new TreeHeapObject(3);
    auto* object2 = new TreeHeapObject(3);
    auto* object3 = new TreeHeapObject(3);
    auto* object4 = new TreeHeapObject(3);

    object1->set_field_object(object2, object2->runtime_object_id, 0);

    object2->set_field_object(object3, object3->runtime_object_id, 0);

    object3->set_field_object(object1, object1->runtime_object_id, 0);

    object2->set_field_object(object4, object4->runtime_object_id, 1);

    object1->hold();
    object2->hold();
    object2->drop();

    heap_manager->show_heap_info();

    object1->drop();

    heap_manager->show_heap_info();

    std::cout << "Complete!\n";
}