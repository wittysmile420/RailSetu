#pragma once

#include "Types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace railsetu {

struct State {
    std::unordered_map<std::string, Train> trains;
    std::unordered_map<std::string, Booking> bookings;
    std::vector<Booking> cancellation_history;
};

class Persistence {
public:
    explicit Persistence(std::string path);
    bool save(const State& state) const;
    bool load(State& state) const;
    bool backup() const;
    bool restore() const;

private:
    std::string path_;
};

} // namespace railsetu
