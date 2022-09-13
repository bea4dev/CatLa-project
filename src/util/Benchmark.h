#pragma once

#include <chrono>

using namespace std::chrono;

namespace benchmark {

    class Timing {
    private:
        system_clock::time_point start_point;
        int64_t time = 0;

    public:
        Timing() = default;

        void start();

        void end();

        int64_t get_sum_time() const;
    };


    extern Timing collect_timing;
    extern Timing release_timing;

}