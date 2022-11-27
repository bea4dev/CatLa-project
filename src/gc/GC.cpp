#include <gc/GC.h>
#include <unordered_map>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->collector_lock = PTHREAD_MUTEX_INITIALIZER;
}

void CycleCollector::gc_collect() {
    //Locking to limit Cycle Collector to one thread.
    pthread_mutex_lock(&collector_lock);

    //Move list
    this->list_lock.lock();
    auto* roots = this->suspected_object_list;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->list_lock.unlock();



    this->list_lock.lock();
    for (auto& root : *roots) {
        this->suspected_object_list->insert(root);
    }
    this->list_lock.unlock();

    delete roots;

    pthread_mutex_unlock(&collector_lock);
}

void CycleCollector::collect_cycles() {

}
