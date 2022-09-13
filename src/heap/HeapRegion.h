#pragma once
#include <cstdint>
#include <util/Concurrent.h>


typedef struct {
    uint32_t flags; //4bit
    atomic_flag lock_flag; //4bit
    size_t refs_length; //8bit
    size_t vals_length; //8bit
    CatLaClass* class_info; //8bit
} HeapClassObject;


class HeapRegion {
private:
    uint8_t* start_position;
    size_t size;

public:
    HeapRegion(size_t size);
    ~HeapRegion();
    void* alloc_for_class(CatLaClass* class_info, size_t refs_length, size_t vals_length);

};