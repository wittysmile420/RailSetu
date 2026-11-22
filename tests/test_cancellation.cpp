#include "railsetu/BookingManager.hpp"
#include <cassert>

int main() {
    railsetu::BookingManager manager("test_cancellation.state", "test_cancellation.log");
    manager.set_persistence_enabled(false);
    assert(manager.add_train({"T-CANCEL", "Cancel", 1, 100, {}}));
    const auto first = manager.book("T-CANCEL", "First", 1);
    const auto second = manager.book("T-CANCEL", "Second", 1);
    assert(manager.cancel(first.pnr).success);
    railsetu::Booking promoted;
    assert(manager.get_booking(second.pnr, promoted));
    assert(promoted.status == railsetu::BookingStatus::Confirmed);
    assert(!manager.cancel(first.pnr).success);
    return 0;
}
