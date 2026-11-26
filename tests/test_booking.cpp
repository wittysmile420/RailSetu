#include "../include/railsetu/BookingManager.hpp"
#include "../include/railsetu/Semaphore.hpp"
#include <atomic>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;

int main() {
    const char* state = "test_railsetu.state";
    const char* log = "test_railsetu.log";
    fs::remove(state);
    fs::remove(log);

    railsetu::BookingManager manager(state, log);
    assert(manager.add_train({"T1", "Test Express", 2, 100, {}}));
    assert(!manager.add_train({"T1", "Duplicate", 99, 100, {}}));

    auto c1 = manager.book("T1", "Alice", 1);
    auto c2 = manager.book("T1", "Bob", 1);
    auto w1 = manager.book("T1", "Carol", 1);
    assert(c1.success && c2.success && w1.success);
    assert(manager.confirmed_seats("T1") == 2);
    assert(manager.waitlist_size("T1") == 1);

    auto canceled = manager.cancel(c1.pnr);
    assert(canceled.success);
    railsetu::Booking promoted;
    assert(manager.get_booking(w1.pnr, promoted));
    assert(promoted.status == railsetu::BookingStatus::Confirmed);
    assert(manager.confirmed_seats("T1") == 2);
    assert(!manager.cancel("RS99999999").success);
    assert(!manager.cancel(c1.pnr).success);

    assert(manager.add_train({"FIFO", "FIFO Express", 1, 100, {}}));
    auto first = manager.book("FIFO", "First", 1);
    auto second = manager.book("FIFO", "Second", 1);
    auto third = manager.book("FIFO", "Third", 1);
    assert(first.success && second.success && third.success);
    assert(manager.cancel(first.pnr).success);
    railsetu::Booking promoted_first;
    assert(manager.get_booking(second.pnr, promoted_first));
    assert(promoted_first.status == railsetu::BookingStatus::Confirmed);
    assert(manager.cancel(second.pnr).success);
    railsetu::Booking promoted_second;
    assert(manager.get_booking(third.pnr, promoted_second));
    assert(promoted_second.status == railsetu::BookingStatus::Confirmed);

    assert(manager.save());
    railsetu::BookingManager restored(state, log);
    assert(restored.load());
    railsetu::Booking loaded;
    assert(restored.get_booking(c2.pnr, loaded));
    assert(loaded.status == railsetu::BookingStatus::Confirmed);

    {
        std::ofstream malformed(state, std::ios::trunc);
        malformed << "RAILSETU_STATE_V1\nT,Broken,Train,not-a-number\n";
    }
    railsetu::BookingManager invalid_state(state, log);
    assert(!invalid_state.load());

    railsetu::Semaphore semaphore(0);
    std::thread releaser([&] { semaphore.release(1); });
    semaphore.acquire();
    releaser.join();
    railsetu::Semaphore bounded(1);
    bool overflow = false;
    try {
        bounded.release(std::numeric_limits<std::size_t>::max());
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    assert(overflow);

    railsetu::BookingManager concurrent("unused.state", "unused.log");
    concurrent.set_persistence_enabled(false);
    concurrent.add_train({"T2", "Concurrent", 100, 100, {}});
    std::atomic<int> successes{0};
    std::vector<std::thread> workers;
    for (int i = 0; i < 32; ++i) {
        workers.emplace_back([&, i] {
            if (concurrent.book("T2", "User" + std::to_string(i), 1).success)
                ++successes;
        });
    }
    for (auto& worker : workers) worker.join();
    assert(successes.load() == 32);
    assert(concurrent.confirmed_seats("T2") == 32);

    railsetu::BookingManager seat_manager("seat_test.state", "seat_test.log");
    seat_manager.set_persistence_enabled(false);
    assert(seat_manager.add_train({"SEAT", "Seat Express", 2, 100, {}}));
    assert(seat_manager.add_coach("SEAT", {"C1", "AC", 2}));
    assert(seat_manager.confirmed_seats("SEAT") == 0);
    auto senior = seat_manager.book("SEAT", "Senior Passenger", 1, railsetu::PassengerCategory::Senior);
    assert(senior.success && !senior.pnr.empty());
    railsetu::Booking senior_booking;
    assert(seat_manager.get_booking(senior.pnr, senior_booking));
    assert(senior_booking.category == railsetu::PassengerCategory::Senior);
    assert(!senior_booking.seat_numbers.empty());
    assert(seat_manager.available_seats("SEAT").size() == 1);
    assert(seat_manager.cancel(senior.pnr).success);
    assert(seat_manager.refund_for(senior.pnr) == senior_booking.fare_paid * 80 / 100);
    assert(seat_manager.cancellation_history().size() == 1);
    assert(seat_manager.wait_for_seats("SEAT", 1, std::chrono::milliseconds(1)));
    assert(seat_manager.unsafe_race_demo(10) <= 80);
    assert(seat_manager.deadlock_demo());
    assert(seat_manager.save());
    assert(seat_manager.backup());

    fs::remove(state);
    fs::remove(log);
    fs::remove("unused.state");
    fs::remove("unused.log");
    fs::remove("seat_test.state");
    fs::remove("seat_test.state.bak");
    fs::remove("seat_test.state.tmp");
    fs::remove("seat_test.log");
    return 0;
}
