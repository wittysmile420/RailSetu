#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace railsetu {

enum class BookingStatus { Confirmed, Waitlisted, Cancelled };
enum class PassengerCategory { General, Ladies, Senior, Tatkal };
enum class CoachType { Sleeper, AC, General };

inline const char* to_string(BookingStatus status) {
    switch (status) {
        case BookingStatus::Confirmed: return "CONFIRMED";
        case BookingStatus::Waitlisted: return "WAITLISTED";
        case BookingStatus::Cancelled: return "CANCELLED";
    }
    return "UNKNOWN";
}

inline const char* to_string(PassengerCategory category) {
    switch (category) {
        case PassengerCategory::General: return "GENERAL";
        case PassengerCategory::Ladies: return "LADIES";
        case PassengerCategory::Senior: return "SENIOR";
        case PassengerCategory::Tatkal: return "TATKAL";
    }
    return "UNKNOWN";
}

inline PassengerCategory category_from_string(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char c : value) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        normalized.push_back(c);
    }
    if (normalized == "LADIES") return PassengerCategory::Ladies;
    if (normalized == "SENIOR") return PassengerCategory::Senior;
    if (normalized == "TATKAL") return PassengerCategory::Tatkal;
    return PassengerCategory::General;
}

inline int priority(PassengerCategory category) {
    switch (category) {
        case PassengerCategory::Senior: return 3;
        case PassengerCategory::Ladies: return 2;
        case PassengerCategory::Tatkal: return 1;
        case PassengerCategory::General: return 0;
    }
    return 0;
}

struct Coach {
    std::string id;
    std::string type = "GENERAL";
    std::uint32_t seat_count = 0;
};

struct Train {
    std::string id;
    std::string name;
    std::uint32_t capacity = 0;
    std::uint32_t base_fare = 100;
    std::vector<Coach> coaches;
};

struct Booking {
    std::string pnr;
    std::string train_id;
    std::string passenger_name;
    std::uint32_t seats = 1;
    std::uint64_t sequence = 0;
    BookingStatus status = BookingStatus::Waitlisted;
    PassengerCategory category = PassengerCategory::General;
    std::string coach_id;
    std::vector<std::string> seat_numbers;
    std::uint64_t fare_paid = 0;
    std::uint64_t refund_amount = 0;
    std::string cancelled_at;
};

struct BookingResult {
    bool success = false;
    std::string pnr;
    std::string message;
};

} // namespace railsetu
