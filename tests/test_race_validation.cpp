#include "railsetu/BookingManager.hpp"
#include <cassert>

int main() {
    railsetu::BookingManager manager("test_race.state", "test_race.log");
    manager.set_persistence_enabled(false);
    assert(manager.add_train({"T-RACE", "Race", 10, 100, {}}));
    for (int i = 0; i < 10; ++i) {
        assert(manager.book("T-RACE", "Safe" + std::to_string(i), 1).success);
    }
    assert(manager.confirmed_seats("T-RACE") == 10);
    assert(manager.unsafe_race_demo(100) > 0);
    assert(manager.deadlock_demo());
    return 0;
}
