#pragma once

#include "Types.hpp"
#include "Semaphore.hpp"
#include "Logger.hpp"
#include "Persistence.hpp"
#include <cstdint>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <vector>

namespace railsetu {

class BookingManager {
public:
    BookingManager(std::string persistence_path, std::string log_path);

    bool add_train(const Train& train);
    BookingResult book(const std::string& train_id,
                       const std::string& passenger_name,
                       std::uint32_t seats);
    BookingResult book(const std::string& train_id,
                       const std::string& passenger_name,
                       std::uint32_t seats,
                       PassengerCategory category);
    bool add_coach(const std::string& train_id, const Coach& coach);
    bool wait_for_seats(const std::string& train_id,
                        std::uint32_t seats,
                        std::chrono::milliseconds timeout);
    BookingResult cancel(const std::string& pnr);
    bool get_booking(const std::string& pnr, Booking& out) const;
    std::vector<Booking> bookings_for(const std::string& passenger_name) const;
    std::vector<Train> trains() const;
    bool train(const std::string& train_id, Train& out) const;
    std::uint32_t confirmed_seats(const std::string& train_id) const;
    std::size_t waitlist_size(const std::string& train_id) const;
    bool save() const;
    bool load();
    void set_persistence_enabled(bool enabled);
    bool backup() const;
    bool restore();
    std::vector<Booking> cancellation_history() const;
    std::vector<std::string> available_seats(const std::string& train_id) const;
    std::vector<Booking> waitlist_entries(const std::string& train_id) const;
    std::vector<Booking> all_bookings() const;
    std::uint64_t refund_for(const std::string& pnr) const;
    void report_benchmark(const std::string& csv_path,
                          std::size_t threads,
                          std::size_t requested,
                          std::size_t confirmed,
                          std::size_t waitlisted,
                          double elapsed_ms) const;
    std::size_t unsafe_race_demo(std::size_t operations);
    bool deadlock_demo();

private:
    mutable std::mutex mutex_;
    std::condition_variable seats_cv_;
    Logger logger_;
    Persistence persistence_;
    std::unordered_map<std::string, Train> trains_;
    std::unordered_map<std::string, Booking> bookings_;
    std::unordered_map<std::string, std::deque<std::string>> waitlists_;
    std::unordered_map<std::string, std::uint32_t> confirmed_by_train_;
    std::uint64_t sequence_ = 0;
    std::vector<Booking> cancellation_history_;
    bool persistence_enabled_ = true;

    std::string next_pnr();
};

} // namespace railsetu
