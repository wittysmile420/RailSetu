#include "../include/railsetu/BookingManager.hpp"
#include "../include/railsetu/Semaphore.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <filesystem>

int main(int argc, char** argv) {
    const int thread_count = argc > 1 ? std::stoi(argv[1]) : 64;
    const int bookings_per_thread = argc > 2 ? std::stoi(argv[2]) : 10;
    railsetu::BookingManager manager("data/railsetu_stress.state", "logs/stress_test.log");
    manager.set_persistence_enabled(false);
    manager.add_train({"STRESS", "Stress Benchmark Train", 100, 100, {}});

    railsetu::Semaphore gate(0);
    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i] {
            gate.acquire();
            for (int j = 0; j < bookings_per_thread; ++j) {
                auto result = manager.book("STRESS", "Passenger-" + std::to_string(i), 1);
                if (!result.success) ++failures;
            }
        });
    }
    gate.release(thread_count);
    const auto start = std::chrono::steady_clock::now();
    for (auto& thread : threads) thread.join();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    railsetu::Train train;
    manager.train("STRESS", train);
    const std::uint32_t confirmed = manager.confirmed_seats("STRESS");
    const std::size_t waitlisted = manager.waitlist_size("STRESS");
    std::cout << "threads=" << thread_count
              << " requested=" << thread_count * bookings_per_thread
              << " confirmed=" << confirmed
              << " waitlisted=" << waitlisted
              << " failures=" << failures.load()
              << " elapsed_ms=" << elapsed.count() << '\n';
    manager.report_benchmark("benchmark/benchmark.csv",
                             static_cast<std::size_t>(thread_count),
                             static_cast<std::size_t>(thread_count * bookings_per_thread),
                             confirmed, waitlisted, static_cast<double>(elapsed.count()));
    std::ofstream summary("benchmark/benchmark_summary.txt", std::ios::trunc);
    summary << "RailSetu Benchmark Summary\n"
            << "threads=" << thread_count << '\n'
            << "requested=" << thread_count * bookings_per_thread << '\n'
            << "confirmed=" << confirmed << '\n'
            << "waitlisted=" << waitlisted << '\n'
            << "elapsed_ms=" << elapsed.count() << '\n';
    if (confirmed > train.capacity || failures.load() != 0) return 1;
    return 0;
}
