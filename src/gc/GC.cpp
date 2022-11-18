#include <gc/GC.h>
#include <unordered_map>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->collector_lock = PTHREAD_MUTEX_INITIALIZER;
}

void CycleCollector::collect_cycles() {
    //Locking to limit Cycle Collector to one thread.
    pthread_mutex_lock(&collector_lock);



    pthread_mutex_unlock(&collector_lock);
}