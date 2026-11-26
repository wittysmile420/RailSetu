#include "../include/railsetu/BookingManager.hpp"
#include <cassert>
#include <thread>
#include <vector>

int main() {
    railsetu::BookingManager manager("test_concurrent.state", "test_concurrent.log");
    manager.set_persistence_enabled(false);
    assert(manager.add_train({"T-CONCURRENT", "Concurrent", 20, 100, {}}));
    std::vector<std::thread> workers;
    for (int i = 0; i < 20; ++i) {
        workers.emplace_back([&manager, i] {
            assert(manager.book("T-CONCURRENT", "P" + std::to_string(i), 1).success);
        });
    }
    for (auto& worker : workers) worker.join();
    assert(manager.confirmed_seats("T-CONCURRENT") == 20);
    return 0;
}
