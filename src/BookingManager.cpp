#include "railsetu/BookingManager.hpp"
#include <algorithm>
#include <atomic>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <utility>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <thread>

namespace railsetu {

namespace {
std::string utc_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&tm, &time);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
}

BookingManager::BookingManager(std::string persistence_path, std::string log_path)
    : logger_(std::move(log_path)), persistence_(std::move(persistence_path)) {}

bool BookingManager::add_train(const Train& train) {
    if (train.id.empty() || train.capacity == 0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (trains_.find(train.id) != trains_.end()) return false;
    trains_[train.id] = train;
    confirmed_by_train_[train.id] = 0;
    logger_.log("INFO", "Added train " + train.id);
    return true;
}

bool BookingManager::add_coach(const std::string& train_id, const Coach& coach) {
    if (coach.id.empty() || coach.seat_count == 0) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto train = trains_.find(train_id);
    if (train == trains_.end()) return false;
    for (const Coach& existing : train->second.coaches)
        if (existing.id == coach.id) return false;
    train->second.coaches.push_back(coach);
    if (train->second.coaches.size() == 1)
        train->second.capacity = coach.seat_count;
    else
        train->second.capacity += coach.seat_count;
    logger_.log("INFO", "coach_added train=" + train_id + " coach=" + coach.id +
                       " type=" + coach.type + " seats=" + std::to_string(coach.seat_count));
    return true;
}

std::string BookingManager::next_pnr() {
    ++sequence_;
    std::ostringstream out;
    out << "RS" << std::setw(8) << std::setfill('0') << sequence_;
    std::string pnr = out.str();
    while (bookings_.find(pnr) != bookings_.end()) {
        ++sequence_;
        out.str("");
        out.clear();
        out << "RS" << std::setw(8) << std::setfill('0') << sequence_;
        pnr = out.str();
    }
    return pnr;
}

BookingResult BookingManager::book(const std::string& train_id,
                                   const std::string& passenger_name,
                                   std::uint32_t seats) {
    return book(train_id, passenger_name, seats, PassengerCategory::General);
}

BookingResult BookingManager::book(const std::string& train_id,
                                   const std::string& passenger_name,
                                   std::uint32_t seats,
                                   PassengerCategory category) {
    BookingResult result;
    bool should_persist = false;
    {
    std::lock_guard<std::mutex> lock(mutex_);
    if (train_id.empty() || passenger_name.empty() || seats == 0) {
        return {false, "", "Invalid booking parameters"};
    }
    auto train_it = trains_.find(train_id);
    if (train_it == trains_.end()) return {false, "", "Train not found"};

    Booking b;
    b.pnr = next_pnr();
    b.train_id = train_id;
    b.passenger_name = passenger_name;
    b.seats = seats;
    b.sequence = sequence_;
    b.category = category;
    b.fare_paid = static_cast<std::uint64_t>(train_it->second.base_fare) * seats;
    const std::uint32_t used = confirmed_by_train_[train_id];
    if (used <= train_it->second.capacity &&
        seats <= train_it->second.capacity - used) {
        b.status = BookingStatus::Confirmed;
        confirmed_by_train_[train_id] = used + seats;
        if (!train_it->second.coaches.empty()) {
            for (const Coach& coach : train_it->second.coaches) {
                std::uint32_t free_count = coach.seat_count;
                for (const auto& occupied : bookings_) {
                    if (occupied.second.status == BookingStatus::Confirmed &&
                        occupied.second.train_id == train_id &&
                        occupied.second.coach_id == coach.id) {
                        free_count -= static_cast<std::uint32_t>(occupied.second.seat_numbers.size());
                    }
                }
                if (free_count >= seats) {
                    b.coach_id = coach.id;
                    std::uint32_t start = 0;
                    for (std::uint32_t seat = 1; seat <= coach.seat_count; ++seat) {
                        std::string label = coach.id + "-" + std::to_string(seat);
                        bool occupied = false;
                        for (const auto& pair : bookings_) {
                            if (pair.second.status == BookingStatus::Confirmed &&
                                pair.second.train_id == train_id) {
                                for (const std::string& assigned : pair.second.seat_numbers)
                                    if (assigned == label) occupied = true;
                            }
                        }
                        if (!occupied) {
                            ++start;
                            b.seat_numbers.push_back(label);
                            if (start == seats) break;
                        }
                    }
                    break;
                }
            }
        }
    } else {
        b.status = BookingStatus::Waitlisted;
        auto& queue = waitlists_[train_id];
        auto insert_at = queue.end();
        for (auto it = queue.begin(); it != queue.end(); ++it) {
            auto waiting = bookings_.find(*it);
            if (waiting != bookings_.end() &&
                railsetu::priority(category) > railsetu::priority(waiting->second.category)) {
                insert_at = it;
                break;
            }
        }
        queue.insert(insert_at, b.pnr);
    }
    bookings_[b.pnr] = b;
    should_persist = persistence_enabled_;

    logger_.log("INFO", b.pnr + " " + to_string(b.status) + " train=" + train_id);
    result = {true, b.pnr,
              b.status == BookingStatus::Confirmed ? "Booking confirmed" : "Added to waitlist"};
    seats_cv_.notify_all();
    }
    if (should_persist && !save()) {
        result.message += " (state persistence failed)";
    }
    return result;
}

bool BookingManager::wait_for_seats(const std::string& train_id,
                                    std::uint32_t seats,
                                    std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto train = trains_.find(train_id);
    if (train == trains_.end() || seats == 0) return false;
    const bool available = seats_cv_.wait_for(lock, timeout, [&] {
        return confirmed_by_train_[train_id] <= train->second.capacity &&
               seats <= train->second.capacity - confirmed_by_train_[train_id];
    });
    return available;
}

BookingResult BookingManager::cancel(const std::string& pnr) {
    BookingResult result;
    bool should_persist = false;
    {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = bookings_.find(pnr);
    if (it == bookings_.end()) return {false, "", "PNR not found"};
    Booking& b = it->second;
    if (b.status == BookingStatus::Cancelled) return {false, "", "Booking already cancelled"};

    if (b.status == BookingStatus::Confirmed) {
        confirmed_by_train_[b.train_id] -= b.seats;
        b.refund_amount = b.fare_paid * 80 / 100;
        b.cancelled_at = utc_timestamp();
    } else {
        auto& queue = waitlists_[b.train_id];
        queue.erase(std::remove(queue.begin(), queue.end(), pnr), queue.end());
        b.cancelled_at = utc_timestamp();
        b.refund_amount = 0;
    }
    b.status = BookingStatus::Cancelled;
    cancellation_history_.push_back(b);
    seats_cv_.notify_all();

    auto train_it = trains_.find(b.train_id);
    if (train_it != trains_.end()) {
        auto& queue = waitlists_[b.train_id];
        while (!queue.empty()) {
            std::string next = queue.front();
            queue.pop_front();
            auto wait_it = bookings_.find(next);
            if (wait_it == bookings_.end()) continue;
            Booking& wait = wait_it->second;
            if (wait.status != BookingStatus::Waitlisted) continue;
            std::uint32_t used = confirmed_by_train_[b.train_id];
            if (used + wait.seats <= train_it->second.capacity) {
                wait.status = BookingStatus::Confirmed;
                confirmed_by_train_[b.train_id] = used + wait.seats;
                wait.seat_numbers.clear();
                for (const Coach& coach : train_it->second.coaches) {
                    std::uint32_t free_count = coach.seat_count;
                    for (const auto& occupied : bookings_) {
                        if (occupied.second.status == BookingStatus::Confirmed &&
                            occupied.second.train_id == wait.train_id &&
                            occupied.second.coach_id == coach.id)
                            free_count -= static_cast<std::uint32_t>(occupied.second.seat_numbers.size());
                    }
                    if (free_count >= wait.seats) {
                        wait.coach_id = coach.id;
                        std::uint32_t start = 0;
                        for (std::uint32_t seat = 1; seat <= coach.seat_count && start < wait.seats; ++seat) {
                            std::string label = coach.id + "-" + std::to_string(seat);
                            bool occupied = false;
                            for (const auto& pair : bookings_) {
                                if (pair.second.status == BookingStatus::Confirmed &&
                                    pair.second.train_id == wait.train_id)
                                    for (const std::string& assigned : pair.second.seat_numbers)
                                        if (assigned == label) occupied = true;
                            }
                            if (!occupied) {
                                ++start;
                                wait.seat_numbers.push_back(label);
                            }
                        }
                        break;
                    }
                }
                logger_.log("INFO", wait.pnr + " promoted to confirmed");
            } else {
                queue.push_front(next);
                break;
            }
        }
    }
    should_persist = persistence_enabled_;
    logger_.log("INFO", pnr + " cancelled");
    result = {true, pnr, "Booking cancelled"};
    }
    if (should_persist && !save()) {
        result.message += " (state persistence failed)";
    }
    return result;
}

bool BookingManager::get_booking(const std::string& pnr, Booking& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = bookings_.find(pnr);
    if (it == bookings_.end()) return false;
    out = it->second;
    return true;
}

std::vector<Booking> BookingManager::bookings_for(const std::string& passenger_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Booking> out;
    for (const auto& pair : bookings_) {
        if (pair.second.passenger_name == passenger_name) out.push_back(pair.second);
    }
    return out;
}

std::vector<Train> BookingManager::trains() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Train> out;
    out.reserve(trains_.size());
    for (const auto& pair : trains_) out.push_back(pair.second);
    std::sort(out.begin(), out.end(), [](const Train& a, const Train& b) {
        return a.id < b.id;
    });
    return out;
}

bool BookingManager::train(const std::string& train_id, Train& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = trains_.find(train_id);
    if (it == trains_.end()) return false;
    out = it->second;
    return true;
}

std::uint32_t BookingManager::confirmed_seats(const std::string& train_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = confirmed_by_train_.find(train_id);
    return it == confirmed_by_train_.end() ? 0 : it->second;
}

std::size_t BookingManager::waitlist_size(const std::string& train_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = waitlists_.find(train_id);
    return it == waitlists_.end() ? 0 : it->second.size();
}

bool BookingManager::save() const {
    State snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = {trains_, bookings_, cancellation_history_};
    }
    return persistence_.save(snapshot);
}

bool BookingManager::load() {
    State state;
    if (!persistence_.load(state)) return false;
    std::unordered_map<std::string, std::uint32_t> confirmed;
    std::unordered_map<std::string, std::deque<std::string>> waitlists;
    std::uint64_t loaded_sequence = 0;
    for (const auto& pair : state.trains) {
        if (pair.first.empty() || pair.second.id != pair.first || pair.second.capacity == 0) {
            return false;
        }
        confirmed[pair.first] = 0;
    }
    std::vector<const Booking*> waiting;
    for (const auto& pair : state.bookings) {
        const Booking& b = pair.second;
        if (b.pnr != pair.first || b.pnr.empty() || b.train_id.empty() ||
            b.passenger_name.empty() || b.seats == 0 ||
            state.trains.find(b.train_id) == state.trains.end()) {
            return false;
        }
        const int status = static_cast<int>(b.status);
        if (status < static_cast<int>(BookingStatus::Confirmed) ||
            status > static_cast<int>(BookingStatus::Cancelled)) {
            return false;
        }
        loaded_sequence = std::max(loaded_sequence, b.sequence);
        if (b.pnr.rfind("RS", 0) == 0) {
            std::uint64_t pnr_sequence = 0;
            const std::string digits = b.pnr.substr(2);
            const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(),
                                                pnr_sequence);
            if (parsed.ec == std::errc{} && parsed.ptr == digits.data() + digits.size()) {
                loaded_sequence = std::max(loaded_sequence, pnr_sequence);
            }
        }
        if (b.status == BookingStatus::Confirmed) {
            const std::uint32_t capacity = state.trains.at(b.train_id).capacity;
            if (b.seats > capacity - confirmed[b.train_id]) return false;
            confirmed[b.train_id] += b.seats;
        } else if (b.status == BookingStatus::Waitlisted) {
            waiting.push_back(&b);
        }
    }
    std::sort(waiting.begin(), waiting.end(), [](const Booking* left, const Booking* right) {
        const int lp = priority(left->category);
        const int rp = priority(right->category);
        return lp != rp ? lp > rp : left->sequence < right->sequence;
    });
    for (const Booking* booking : waiting) {
        waitlists[booking->train_id].push_back(booking->pnr);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        trains_ = std::move(state.trains);
        bookings_ = std::move(state.bookings);
        waitlists_ = std::move(waitlists);
        confirmed_by_train_ = std::move(confirmed);
        cancellation_history_ = std::move(state.cancellation_history);
        sequence_ = loaded_sequence;
    }
    logger_.log("INFO", "State loaded");
    return true;
}

void BookingManager::set_persistence_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    persistence_enabled_ = enabled;
}

bool BookingManager::backup() const {
    return persistence_.backup();
}

bool BookingManager::restore() {
    if (!persistence_.restore()) return false;
    return load();
}

std::vector<Booking> BookingManager::cancellation_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cancellation_history_;
}

std::uint64_t BookingManager::refund_for(const std::string& pnr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = bookings_.find(pnr);
    return it == bookings_.end() ? 0 : it->second.refund_amount;
}

std::vector<std::string> BookingManager::available_seats(const std::string& train_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    auto train = trains_.find(train_id);
    if (train == trains_.end()) return result;
    for (const Coach& coach : train->second.coaches) {
        for (std::uint32_t seat = 1; seat <= coach.seat_count; ++seat) {
            const std::string label = coach.id + "-" + std::to_string(seat);
            bool occupied = false;
            for (const auto& pair : bookings_) {
                if (pair.second.status == BookingStatus::Confirmed &&
                    pair.second.train_id == train_id)
                    for (const std::string& assigned : pair.second.seat_numbers)
                        if (assigned == label) occupied = true;
            }
            if (!occupied) result.push_back(label);
        }
    }
    return result;
}

std::vector<Booking> BookingManager::waitlist_entries(const std::string& train_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Booking> result;
    auto queue = waitlists_.find(train_id);
    if (queue == waitlists_.end()) return result;
    for (const std::string& pnr : queue->second) {
        auto booking = bookings_.find(pnr);
        if (booking != bookings_.end()) result.push_back(booking->second);
    }
    return result;
}

std::vector<Booking> BookingManager::all_bookings() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Booking> result;
    result.reserve(bookings_.size());
    for (const auto& pair : bookings_) result.push_back(pair.second);
    std::sort(result.begin(), result.end(), [](const Booking& a, const Booking& b) {
        return a.sequence < b.sequence;
    });
    return result;
}

void BookingManager::report_benchmark(const std::string& csv_path,
                                      std::size_t threads,
                                      std::size_t requested,
                                      std::size_t confirmed,
                                      std::size_t waitlisted,
                                      double elapsed_ms) const {
    namespace fs = std::filesystem;
    fs::path path(csv_path);
    if (path.has_parent_path()) fs::create_directories(path.parent_path());
    const bool exists = fs::exists(path);
    std::ofstream out(path, std::ios::app);
    if (!exists) out << "threads,requested,confirmed,waitlisted,elapsed_ms\n";
    out << threads << ',' << requested << ',' << confirmed << ',' << waitlisted
        << ',' << elapsed_ms << '\n';
}

std::size_t BookingManager::unsafe_race_demo(std::size_t operations) {
    // Deliberately isolated: every worker reads the same availability snapshot
    // before allocating from it. This reproduces stale-read behavior without
    // touching real state or creating undefined C++ shared-memory access.
    std::atomic<std::size_t> requested{operations};
    std::atomic<std::size_t> allocated{0};
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < 8; ++i) {
        threads.emplace_back([&requested, &allocated, operations] {
            for (std::size_t n = 0; n < operations; ++n) {
                // Availability is read outside a reservation critical section.
                const std::size_t stale_available = requested.load();
                if (stale_available > 0) {
                    allocated.fetch_add(1);
                    requested.fetch_sub(1);
                }
            }
        });
    }
    for (auto& thread : threads) thread.join();
    return allocated.load();
}

bool BookingManager::deadlock_demo() {
    std::mutex first;
    std::mutex second;
    bool completed = false;
    std::thread a([&] {
        std::lock(first, second);
        std::lock_guard<std::mutex> a1(first, std::adopt_lock);
        std::lock_guard<std::mutex> a2(second, std::adopt_lock);
        completed = true;
    });
    std::thread b([&] {
        std::lock(first, second);
        std::lock_guard<std::mutex> b1(first, std::adopt_lock);
        std::lock_guard<std::mutex> b2(second, std::adopt_lock);
    });
    a.join();
    b.join();
    return completed;
}

} // namespace railsetu
