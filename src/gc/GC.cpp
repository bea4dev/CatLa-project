#include <gc/GC.h>
#include <unordered_map>

using namespace gc;

CycleCollector::CycleCollector(void* vm) {
    this->vm = vm;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->collector_lock = PTHREAD_MUTEX_INITIALIZER;
}

void CycleCollector::process_cycles() {
    //Locking to limit Cycle Collector to one thread.
    pthread_mutex_lock(&collector_lock);

    //Move list
    this->list_lock.lock();
    auto* roots = this->suspected_object_list;
    this->suspected_object_list = new unordered_set<HeapObject*>;
    this->list_lock.unlock();

    free_cycles(roots);
    collect_cycles(roots);
    sigma_preparation();

    this->list_lock.lock();
    for (auto& root : *roots) {
        this->suspected_object_list->insert(root);
    }
    this->list_lock.unlock();

    delete roots;

    pthread_mutex_unlock(&collector_lock);
}

void CycleCollector::collect_cycles(unordered_set<HeapObject*>* roots) {
    mark_roots(roots);
    scan_roots(roots);
    collect_roots(roots);
}

void CycleCollector::mark_roots(unordered_set<HeapObject*>* roots) {
    unordered_set<HeapObject*> remove;
    for (auto& s : *roots) {
        uint32_t s_color = s->color.load(std::memory_order_acquire);
        size_t s_rc = s->count.load(std::memory_order_acquire);
        if ((s_color == object_color::purple || s_color == object_color::black) && s_rc > 0) {
            mark_gray(s);
        } else {
            remove.insert(s);
            s->buffered.store(0, std::memory_order_release);
            if (s_rc == 0) {
                //Free
                s->color.store(object_color::dead, std::memory_order_release);
            } else {
                //retry.push_back(s);
            }
        }
    }
    for (auto& s : remove) {
        roots->erase(s);
    }
}

void CycleCollector::mark_gray(HeapObject* s) {
    if (s->color.load(std::memory_order_acquire) != object_color::gray) {
        s->color.store(object_color::gray, std::memory_order_release);
        s->crc = s->count.load(std::memory_order_acquire);
        //gray.insert(s);

        auto* current_object = s;
        stack<HeapObject*> check_objects;
        while (true) {
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    if (field_object->color.load(std::memory_order_acquire) != object_color::gray) {
                        field_object->color.store(object_color::gray, std::memory_order_release);
                        field_object->crc = field_object->count.load(std::memory_order_acquire) - 1;
                        check_objects.push(field_object);
                        //gray.insert(field_object);
                    } else if (field_object->crc > 0) {
                        (field_object->crc)--;
                    }
                }
            }

            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }
    } else if (s->crc > 0) {
        (s->crc)--;
    }
}

void CycleCollector::scan_roots(unordered_set<HeapObject*>* roots) {
    for (auto& s : *roots) {
        scan(s);
    }
}

void CycleCollector::scan(HeapObject* s) {
    if (s->color.load(std::memory_order_acquire) == object_color::gray && s->crc == 0) {
        s->color.store(object_color::white, std::memory_order_release);

        auto* current_object = s;
        stack<HeapObject*> check_objects;
        while (true) {
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    if (field_object->color.load(std::memory_order_acquire) == object_color::gray && field_object->crc == 0) {
                        field_object->color.store(object_color::white, std::memory_order_release);
                        check_objects.push(field_object);
                        //white.insert(field_object);
                    } else if (field_object->crc != 0) {
                        scan_black(this, field_object);
                    }
                }
            }

            if (check_objects.empty()) {
                break;
            }
            current_object = check_objects.top();
            check_objects.pop();
        }
    } else {
        scan_black(this, s);
    }
}

void CycleCollector::collect_roots(unordered_set<HeapObject*>* roots) {
    for (auto& s : *roots) {
        if (s->color.load(std::memory_order_acquire) == object_color::white) {
            unordered_set<HeapObject*> current_cycle;
            this->collect_white(s, current_cycle);
            this->cycle_buffer.push_back(std::move(current_cycle));
        } else {
            s->buffered.store(0, std::memory_order_release);
        }
    }
    roots->clear();
}

void CycleCollector::collect_white(HeapObject* s, unordered_set<HeapObject*>& current_cycle) {
    if (s->color.load(std::memory_order_acquire) == object_color::white) {
        s->color.store(object_color::orange, std::memory_order_release);
        s->buffered.store(1, std::memory_order_release);
        current_cycle.insert(s);

        auto* current_object = s;
        stack<HeapObject*> check_objects;
        while (true) {
            auto** fields = (HeapObject**) (current_object + 1);
            size_t field_length = current_object->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* field_object = fields[i];
                if (field_object != nullptr) {
                    if (field_object->color.load(std::memory_order_acquire) == object_color::white) {
                        field_object->color.store(object_color::orange, std::memory_order_release);
                        field_object->buffered.store(1, std::memory_order_release);
                        current_cycle.insert(field_object);
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
    }
}

void CycleCollector::free_cycles(unordered_set<HeapObject*>* roots) {
    if (this->cycle_buffer.empty()) {
        return;
    }
    size_t last = this->cycle_buffer.size();
    for (size_t i = last; i > 0; i--) {
        auto c = this->cycle_buffer[i - 1];
        if (delta_test(c) && sigma_test(c)) {
            free_cycle(c);
        } else {
            refurbish(roots, c);
        }
    }
    this->cycle_buffer.clear();
}

bool CycleCollector::delta_test(unordered_set<HeapObject*>& c) {
    for (auto& n : c) {
        if (n->color.load(std::memory_order_acquire) != object_color::orange) {
            return false;
        }
    }
    return true;
}

bool CycleCollector::sigma_test(unordered_set<HeapObject*>& c) {
    size_t extern_rc = 0;
    for (auto& n : c) {
        extern_rc += n->crc;
    }
    return (extern_rc == 0);
}

void CycleCollector::free_cycle(unordered_set<HeapObject*>& c) {
    for (auto& n : c) {
        n->color.store(object_color::red, std::memory_order_release);
    }
    for (auto& n : c) {
        auto** fields = (HeapObject**) (n + 1);
        size_t field_length = n->field_length;
        for (size_t i = 0; i < field_length; i++) {
            auto* m = fields[i];
            if (m != nullptr) {
                cyclic_decrement(m);
            }
        }
    }
    for (auto& n : c) {
        //Free
        n->color.store(object_color::dead, std::memory_order_release);
    }
}

void CycleCollector::cyclic_decrement(HeapObject* m) {
    uint32_t m_color = m->color.load(std::memory_order_acquire);
    if (m_color != object_color::red) {
        if (m_color == object_color::orange) {
            m->count.fetch_sub(1, std::memory_order_release);
            (m->crc)--;
        } else {
            decrement_reference_count(this, m);
        }
    }
}

void CycleCollector::refurbish(unordered_set<HeapObject*>* roots, unordered_set<HeapObject*>& c) {
    bool first = true;
    for (auto& n : c) {
        uint32_t n_color = n->color.load(std::memory_order_acquire);
        if ((first && n_color == object_color::orange) || n_color == object_color::purple) {
            n->color.store(object_color::purple, std::memory_order_release);
            atomic_thread_fence(std::memory_order_acquire);
            roots->insert(n);
        } else {
            n->color.store(object_color::black, std::memory_order_release);
            n->buffered.store(0, std::memory_order_release);
        }
        first = false;
    }
}

void CycleCollector::sigma_preparation() {
    for (auto& c : this->cycle_buffer) {
        for (auto& n : c) {
            n->color.store(object_color::red, std::memory_order_release);
            n->crc = n->count.load(std::memory_order_relaxed);
        }
        for (auto& n : c) {
            auto** fields = (HeapObject**) (n + 1);
            size_t field_length = n->field_length;
            for (size_t i = 0; i < field_length; i++) {
                auto* m = fields[i];
                if (m != nullptr) {
                    if (m->color.load(std::memory_order_acquire) == object_color::red && m->crc > 0) {
                        (m->crc)--;
                    }
                }
            }
        }
        for (auto& n : c) {
            n->color.store(object_color::orange, std::memory_order_release);
        }
    }
}

void gc::possible_roots(CycleCollector* cycle_collector, HeapObject* object) {
    scan_black(cycle_collector, object);
    object->color.store(object_color::purple, std::memory_order_release);
    uint32_t expected = 0;
    if (object->buffered.compare_exchange_strong(expected, 1, std::memory_order_seq_cst)) {
        cycle_collector->add_suspected_object(object);
        /*
        cycle_collector->collect_lock.write_lock();
        cycle_collector->suspected.insert(object);
        cycle_collector->collect_lock.write_unlock();*/
    }
}

void gc::release(CycleCollector* cycle_collector, HeapObject* object) {
    auto** fields = (HeapObject**) (object + 1);
    size_t field_length = object->field_length;
    for (size_t i = 0; i < field_length; i++) {
        auto* field_object = fields[i];
        if (field_object != nullptr) {
            decrement_reference_count(cycle_collector, field_object);
        }
    }
    object->color.store(object_color::black, std::memory_order_release);
    atomic_thread_fence(std::memory_order_acquire);
    if (!object->buffered.load(std::memory_order_acquire)) {
        //Free
        object->color.store(object_color::dead, std::memory_order_release);
    }
}