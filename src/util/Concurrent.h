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

}