#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <cstdint>
#include <utility>
#include <vector>

namespace railsetu {

class ConsoleUi {
public:
    static void clear();
    static void banner();
    static void menu();
    static void section(std::string_view title);
    static void success(std::string_view message);
    static void error(std::string_view message);
    static void info(std::string_view message);
    static bool read_number(std::string_view prompt, int& value);
    static bool read_unsigned(std::string_view prompt, unsigned int& value);
    static void startup();
    static void dashboard(std::string_view train_count,
                          std::string_view confirmed,
                          std::string_view waitlisted,
                          std::string_view bookings);
    static void footer();
    static void suggestions();
    static bool confirm(std::string_view prompt);
    static void route(std::string_view origin, std::string_view destination);
    static void occupancy(std::string_view label, std::size_t used, std::size_t capacity);
    static void seat_grid(std::string_view title,
                          const std::vector<std::string>& confirmed,
                          const std::vector<std::string>& reserved,
                          const std::vector<std::string>& promotion);
    static void benchmark_chart(std::string_view title,
                                const std::vector<std::pair<std::string, std::size_t>>& bars);
    static void ticket_card(const std::string& pnr,
                            const std::string& passenger,
                            const std::string& train,
                            const std::string& route,
                            const std::string& coach,
                            const std::vector<std::string>& seats,
                            const std::string& status,
                            const std::string& category,
                            std::uint64_t fare);
    static void waitlist_ladder(const std::vector<std::pair<std::string, std::string>>& entries);
    static void activity_feed(const std::vector<std::string>& entries);
    static void booking_progress(std::string_view action);
};

}
