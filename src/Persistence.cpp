#include "railsetu/Persistence.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace railsetu {

namespace {
std::string escape_csv(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        if (c == ',' || c == '\n' || c == '\r') out += ' ';
        else out += c;
    }
    return out;
}

bool split(std::string line, std::vector<std::string>& fields) {
    fields.clear();
    std::size_t start = 0;
    while (start <= line.size()) {
        const std::size_t comma = line.find(',', start);
        fields.push_back(line.substr(start, comma - start));
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return true;
}
}

Persistence::Persistence(std::string path) : path_(std::move(path)) {}

bool Persistence::save(const State& state) const {
    fs::path target(path_);
    if (target.has_parent_path()) fs::create_directories(target.parent_path());
    fs::path temp = target;
    temp += ".tmp";
    std::ofstream out(temp, std::ios::trunc);
    if (!out) return false;
    out << "RAILSETU_STATE_V2\n";
    for (const auto& pair : state.trains) {
        const Train& t = pair.second;
        out << "T," << escape_csv(t.id) << ',' << escape_csv(t.name) << ',' << t.capacity
            << ',' << t.base_fare << '\n';
        for (const auto& coach : t.coaches) {
            out << "C," << escape_csv(t.id) << ',' << escape_csv(coach.id) << ','
                << escape_csv(coach.type) << ',' << coach.seat_count << '\n';
        }
    }
    for (const auto& pair : state.bookings) {
        const Booking& b = pair.second;
        out << "B," << escape_csv(b.pnr) << ',' << escape_csv(b.train_id) << ','
            << escape_csv(b.passenger_name) << ',' << b.seats << ','
            << b.sequence << ',' << static_cast<int>(b.status) << ','
            << static_cast<int>(b.category) << ',' << escape_csv(b.coach_id) << ','
            << b.fare_paid << ',' << b.refund_amount << ',' << escape_csv(b.cancelled_at) << ',';
        for (const auto& seat : b.seat_numbers) out << ';' << escape_csv(seat);
        out << '\n';
    }
    for (const Booking& b : state.cancellation_history) {
        out << "H," << escape_csv(b.pnr) << ',' << escape_csv(b.train_id) << ','
            << escape_csv(b.passenger_name) << ',' << b.seats << ',' << b.refund_amount << ','
            << escape_csv(b.cancelled_at) << '\n';
    }
    out.close();
    if (!out) return false;
    std::error_code ec;
    fs::path backup = target;
    backup += ".bak";
    fs::remove(backup, ec);
    ec.clear();
    if (fs::exists(target, ec)) {
        fs::copy_file(target, backup, fs::copy_options::overwrite_existing, ec);
        if (ec) return false;
    }
    ec.clear();
    fs::remove(target, ec);
    if (ec) return false;
    fs::rename(temp, target, ec);
    return !ec;
}

bool Persistence::load(State& state) const {
    state.trains.clear();
    state.bookings.clear();
    state.cancellation_history.clear();
    std::ifstream in(path_);
    if (!in) return false;
    std::string line;
    if (!std::getline(in, line) ||
        (line != "RAILSETU_STATE_V1" && line != "RAILSETU_STATE_V2")) return false;
    const bool v2 = line == "RAILSETU_STATE_V2";
    try {
    while (std::getline(in, line)) {
        std::vector<std::string> f;
        split(line, f);
        if (f.empty()) continue;
        if (f[0] == "T" && f.size() >= 4) {
            Train t;
            t.id = f[1];
            t.name = f[2];
            t.capacity = static_cast<std::uint32_t>(std::stoul(f[3]));
            t.base_fare = v2 && f.size() >= 5
                ? static_cast<std::uint32_t>(std::stoul(f[4])) : 100;
            state.trains[t.id] = t;
        } else if (f[0] == "C" && v2 && f.size() >= 5) {
            auto train = state.trains.find(f[1]);
            if (train == state.trains.end()) return false;
            Coach coach;
            coach.id = f[2];
            coach.type = f[3];
            coach.seat_count = static_cast<std::uint32_t>(std::stoul(f[4]));
            train->second.coaches.push_back(coach);
        } else if (f[0] == "B" && f.size() >= 7) {
            Booking b;
            b.pnr = f[1];
            b.train_id = f[2];
            b.passenger_name = f[3];
            b.seats = static_cast<std::uint32_t>(std::stoul(f[4]));
            b.sequence = std::stoull(f[5]);
            const int status = std::stoi(f[6]);
            if (status < static_cast<int>(BookingStatus::Confirmed) ||
                status > static_cast<int>(BookingStatus::Cancelled)) {
                state = {};
                return false;
            }
            b.status = static_cast<BookingStatus>(status);
            if (v2 && f.size() >= 13) {
                const int category = std::stoi(f[7]);
                if (category < 0 || category > 3) { state = {}; return false; }
                b.category = static_cast<PassengerCategory>(category);
                b.coach_id = f[8];
                b.fare_paid = std::stoull(f[9]);
                b.refund_amount = std::stoull(f[10]);
                b.cancelled_at = f[11];
                std::stringstream seats(f[12]);
                std::string seat;
                while (std::getline(seats, seat, ';')) if (!seat.empty()) b.seat_numbers.push_back(seat);
            }
            state.bookings[b.pnr] = b;
        } else if (f[0] == "H" && v2 && f.size() >= 7) {
            Booking b;
            b.pnr = f[1];
            b.train_id = f[2];
            b.passenger_name = f[3];
            b.seats = static_cast<std::uint32_t>(std::stoul(f[4]));
            b.refund_amount = std::stoull(f[5]);
            b.cancelled_at = f[6];
            b.status = BookingStatus::Cancelled;
            state.cancellation_history.push_back(std::move(b));
        }
    }
    } catch (const std::invalid_argument&) {
        state = {};
        return false;
    } catch (const std::out_of_range&) {
        state = {};
        return false;
    }
    return true;
}

bool Persistence::backup() const {
    fs::path target(path_);
    if (!fs::exists(target)) return false;
    fs::path backup = target;
    backup += ".bak";
    std::error_code ec;
    fs::remove(backup, ec);
    ec.clear();
    fs::copy_file(target, backup, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

bool Persistence::restore() const {
    fs::path target(path_);
    fs::path backup = target;
    backup += ".bak";
    if (!fs::exists(backup)) return false;
    std::error_code ec;
    fs::remove(target, ec);
    ec.clear();
    fs::rename(backup, target, ec);
    return !ec;
}

} // namespace railsetu
