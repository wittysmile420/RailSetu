#include "railsetu/BookingManager.hpp"
#include <cassert>

int main() {
    railsetu::BookingManager manager("test_waiting.state", "test_waiting.log");
    manager.set_persistence_enabled(false);
    assert(manager.add_train({"T-WAIT", "Wait", 1, 100, {}}));
    const auto confirmed = manager.book("T-WAIT", "Confirmed", 1);
    const auto general = manager.book("T-WAIT", "General", 1,
                                      railsetu::PassengerCategory::General);
    const auto senior = manager.book("T-WAIT", "Senior", 1,
                                     railsetu::PassengerCategory::Senior);
    assert(manager.cancel(confirmed.pnr).success);
    railsetu::Booking result;
    assert(manager.get_booking(senior.pnr, result));
    assert(result.status == railsetu::BookingStatus::Confirmed);
    assert(manager.get_booking(general.pnr, result));
    assert(result.status == railsetu::BookingStatus::Waitlisted);
    return 0;
}
