#include "Benchmark.h"

using namespace benchmark;

void Timing::start() {
    this->start_point = system_clock::now();
}

void Timing::end() {
    auto end = system_clock::now();
    this->time += std::chrono::duration_cast<std::chrono::milliseconds>(end - this->start_point).count();
}

int64_t Timing::get_sum_time() const {
    return this->time;
}


benchmark::Timing benchmark::collect_timing;
benchmark::Timing benchmark::release_timing;
