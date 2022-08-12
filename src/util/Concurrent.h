#pragma once

#include <vm/CatVM.h>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

using namespace std;
using namespace cat_vm;

namespace concurrent {

    class SpinLock {
    private:
        std::atomic_flag flag;

    public:
        SpinLock() = default;

        void lock() {
            while (flag.test_and_set(std::memory_order_acquire)) {
                //wait
            }
        }

        void unlock() {
            flag.clear(std::memory_order_release);
        }
    };


    template<typename K, typename V>
    class SafeOnWriteMap {

    private:
        unordered_map<K, V>** local_maps;
        mutex lock;
        unordered_map<K, V>* global_map;

    public:
        SafeOnWriteMap() {
            local_maps = new unordered_map<K, V>*[reserved_threads];
            global_map = nullptr;

            for (size_t i = 0; i < reserved_threads; i++) {
                local_maps[i] = nullptr;
            }
        }

        ~SafeOnWriteMap() {
            delete[] local_maps;
            delete global_map;
        }

        V get(size_t thread_id, K key) {
            if (thread_id >= reserved_threads) {
                lock.lock();
                if (global_map == nullptr) {
                    global_map = new unordered_map<K, V>;
                    lock.unlock();
                    return nullptr;
                }

                V value = global_map->at(key);
                lock.unlock();
                return value;
            } else {
                unordered_map<V, K>* map = local_maps[thread_id];
                if (map == nullptr) {
                    map = new unordered_map<K, V>;
                    local_maps[thread_id] = map;
                }
                return map->at(key);
            }
        }

        void put(size_t thread_id, K key, V value) {
            if (thread_id >= reserved_threads) {
                lock.lock();
                if (global_map == nullptr) {
                    global_map = new unordered_map<K, V>;
                }
                (*global_map)[key] = value;
                lock.unlock();
            } else {
                unordered_map<K, V>* map = local_maps[thread_id];
                if (map == nullptr) {
                    map = new unordered_map<K, V>;
                    local_maps[thread_id] = map;
                }
                (*map)[key] = value;
            }
        }

        unordered_map<K, V> get_merged_map_unsafe() {
            unordered_map<K, V> map;

            for (size_t i = 0; i < reserved_threads; i++) {
                auto* local_map = local_maps[i];
                if (local_map != nullptr) {
                    for (auto it = local_map->begin(); it != local_map->end(); ++it) {
                        map[it->first] = it->second;
                    }
                }

                if (global_map != nullptr) {
                    for (auto it = global_map->begin(); it != global_map->end(); ++it) {
                        map[it->first] = it->second;
                    }
                }
            }

            return map;
        }

    };

}