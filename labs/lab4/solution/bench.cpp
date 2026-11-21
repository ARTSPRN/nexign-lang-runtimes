#include <atomic>
#include <barrier>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sched.h>
#include <string_view>
#include <thread>

constexpr std::size_t CACHE_LINE_SIZE = 64;
constexpr std::uint64_t ITERATIONS = 100'000'000;

using Counter = std::atomic<std::uint64_t>;
using Clock = std::chrono::steady_clock;

struct alignas(CACHE_LINE_SIZE) AdjacentCounters {
    Counter counters[2]{};
};

struct alignas(CACHE_LINE_SIZE) PaddedCounter {
    Counter value{0};
};

static_assert(sizeof(Counter) == 8 && Counter::is_always_lock_free);
static_assert(sizeof(PaddedCounter) == CACHE_LINE_SIZE);

int run(Counter& first, Counter& second, const int (&cpus)[2]) {
    Clock::time_point start;
    std::barrier ready(3, [&]() noexcept { start = Clock::now(); });
    int before[2];
    int after[2];

    auto worker = [&](Counter* counter, int i) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(cpus[i], &mask);
        if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
            std::perror("sched_setaffinity");
            std::exit(1);
        }
        before[i] = sched_getcpu();
        ready.arrive_and_wait();
        for (std::uint64_t j = 0; j < ITERATIONS; ++j) {
            counter->fetch_add(1, std::memory_order_relaxed);
        }
        after[i] = sched_getcpu();
    };

    std::thread t0(worker, &first, 0);
    std::thread t1(worker, &second, 1);
    ready.arrive_and_wait();
    t0.join();
    t1.join();
    auto end = Clock::now();

    auto first_addr = reinterpret_cast<std::uintptr_t>(&first);
    auto second_addr = reinterpret_cast<std::uintptr_t>(&second);
    auto first_count = first.load(std::memory_order_relaxed);
    auto second_count = second.load(std::memory_order_relaxed);
    std::cout << std::fixed << std::setprecision(3)
              << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";
    std::cout << "CPUs (before -> after): " << before[0] << " -> " << after[0]
              << ", " << before[1] << " -> " << after[1] << '\n';
    std::cout << "Counter addresses: 0x" << std::hex << first_addr << ", 0x" << second_addr
              << std::dec << ", offsets in cache line: " << first_addr % CACHE_LINE_SIZE
              << ", " << second_addr % CACHE_LINE_SIZE << '\n';
    std::cout << "Counts: " << first_count << ", " << second_count << '\n';

    return !(first_count == ITERATIONS && second_count == ITERATIONS &&
             before[0] == cpus[0] && after[0] == cpus[0] &&
             before[1] == cpus[1] && after[1] == cpus[1]);
}

int main(int argc, char** argv) {
    std::string_view mode = argc == 2 ? argv[1] : "";
    if (mode != "adjacent" && mode != "padded") {
        std::cerr << "Usage: " << argv[0] << " adjacent|padded\n";
        return 1;
    }

    cpu_set_t allowed;
    if (sched_getaffinity(0, sizeof(allowed), &allowed) != 0) {
        std::perror("sched_getaffinity");
        return 1;
    }
    int cpus[2];
    int found = 0;
    for (int cpu = 0; cpu < CPU_SETSIZE && found < 2; ++cpu) {
        if (CPU_ISSET(cpu, &allowed)) {
            cpus[found++] = cpu;
        }
    }
    if (found < 2) {
        std::cerr << "Use taskset to allow two CPUs on different cores\n";
        return 1;
    }

    if (mode == "adjacent") {
        AdjacentCounters counters;
        return run(counters.counters[0], counters.counters[1], cpus);
    }
    PaddedCounter counters[2];
    return run(counters[0].value, counters[1].value, cpus);
}
