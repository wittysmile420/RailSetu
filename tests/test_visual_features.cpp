#include "railsetu/BookingManager.hpp"
#include <cassert>

int main() {
    railsetu::BookingManager manager("test_visual.state", "test_visual.log");
    manager.set_persistence_enabled(false);
    assert(manager.add_train({"VIS-101", "Visual Test", 1, 100, {}}));
    assert(manager.add_coach("VIS-101", {"V1", "GENERAL", 1}));
    const auto confirmed = manager.book("VIS-101", "Confirmed", 1,
                                        railsetu::PassengerCategory::General);
    assert(confirmed.success);
    const auto senior = manager.book("VIS-101", "Senior", 1,
                                     railsetu::PassengerCategory::Senior);
    assert(senior.success);
    assert(manager.waitlist_entries("VIS-101").size() == 1);
    assert(manager.all_bookings().size() == 2);
    assert(manager.available_seats("VIS-101").empty());
    return 0;
}
