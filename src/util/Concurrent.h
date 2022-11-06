#pragma once

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

using namespace std;

namespace concurrent {

    class SpinLock {
    private:
        std::atomic_flag flag;

    public:
        SpinLock() = default;

        inline void lock() {
            while (flag.test_and_set(std::memory_order_acquire)) {
                //wait
            }
        }

        inline void unlock() {
            flag.clear(std::memory_order_release);
        }
    };


    class RWLock {
    private:
        //count == 0 : write lock
        //count == 1 : not lock
        //count >= 2 : read lock
        volatile size_t count = 1;

    public:
        inline void read_lock() {
            size_t count_temp;
            while (true) {
                count_temp = this->count;
                if (count_temp == 0) {
                    continue;
                }
                if (((atomic_size_t*) &this->count)->compare_exchange_weak(count_temp, count_temp + 1)) {
                    break;
                }
            }
        }

        inline void read_unlock() {
            ((atomic_size_t*) &this->count)->fetch_sub(1, std::memory_order_release);
        }

        inline void write_lock() {
            size_t count_temp;
            while (true) {
                count_temp = this->count;
                if (count_temp != 1) {
                    continue;
                }
                if (((atomic_size_t*) &this->count)->compare_exchange_weak(count_temp, 0)) {
                    break;
                }
            }
        }

        inline void write_unlock() {
            ((atomic_size_t*) &this->count)->fetch_add(1, std::memory_order_release);
        }
    };

}