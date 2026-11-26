#include "../include/railsetu/BookingManager.hpp"
#include <cassert>
#include <filesystem>

int main() {
    const std::string path = "test_persistence.state";
    railsetu::BookingManager manager(path, "test_persistence.log");
    assert(manager.add_train({"T-PERSIST", "Persist", 2, 100, {}}));
    const auto booking = manager.book("T-PERSIST", "Persistent", 1);
    assert(booking.success && manager.save());
    railsetu::BookingManager restored(path, "test_persistence.log");
    assert(restored.load());
    railsetu::Booking loaded;
    assert(restored.get_booking(booking.pnr, loaded));
    assert(loaded.status == railsetu::BookingStatus::Confirmed);
    std::filesystem::remove(path);
    std::filesystem::remove(path + ".bak");
    return 0;
}
