#include "../include/railsetu/ConsoleUi.hpp"
#include <cstdlib>
#include <limits>

namespace railsetu {

namespace {
constexpr const char* reset = "\033[0m";
constexpr const char* cyan = "\033[36m";
constexpr const char* green = "\033[32m";
constexpr const char* yellow = "\033[33m";
constexpr const char* red = "\033[31m";
constexpr const char* bold = "\033[1m";
constexpr const char* dim = "\033[2m";

bool ansi_enabled() {
#ifdef _WIN32
    return std::getenv("TERM") != nullptr || std::getenv("WT_SESSION") != nullptr ||
           std::getenv("ANSICON") != nullptr;
#else
    return std::getenv("NO_COLOR") == nullptr;
#endif
}
}

void ConsoleUi::clear() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void ConsoleUi::banner() {
    const bool ansi = ansi_enabled();
    const char* c = ansi ? cyan : "";
    const char* b = ansi ? bold : "";
    const char* r = ansi ? reset : "";
    std::cout << c << b
              << "  +======================================================+\n"
              << "  |  ____   _   _  _ ____  _  _  ____  _  _             |\n"
              << "  | | __ ) / \\ | || | ___|| || ||_  _|| || |            |\n"
              << "  | |  _ \\ / _ \\| || |___ \\| || |  | |  | || |_          |\n"
              << "  | | |_) / ___ \\| || |___) | ||_|  | |  |__   _|       |\n"
              << "  | |____/_/   \\_\\_||_|____/|_||_|  |_|     |_|         |\n"
              << "  |                                                      |\n"
              << "  |  RAILSETU  |  SYNCHRONIZATION-SAFE RESERVATION      |\n"
              << "  +======================================================+" << r << "\n";
}

void ConsoleUi::menu() {
    const bool ansi = ansi_enabled();
    const char* c = ansi ? cyan : "";
    const char* b = ansi ? bold : "";
    const char* r = ansi ? reset : "";
    section("MAIN OPERATIONS");
    std::cout << b << c
              << "  [01] Search trains       [02] List all trains      [03] Train details\n"
              << "  [04] Search seats        [05] Book ticket          [06] Concurrent booking\n"
              << "  [07] Cancel booking      [08] PNR lookup           [09] Confirmed bookings\n"
              << "  [10] Waiting list        [11] Promote manually     [12] Remove waiting PNR\n"
              << "  [13] Real-time seat map  [14] Activity log         [15] Stress test\n"
              << "  [16] Fairness test       [17] Benchmark suite      [18] Deadlock prevention\n"
              << "  [19] Unsafe race demo    [20] Export report        [21] Reset demo state\n"
              << "  [22] Exit application    [23] Live dashboard       [24] Route visualization\n"
              << "  [25] Waitlist ladder     [26] Log viewer            [27] Captured demo help\n"
              << r;
}

void ConsoleUi::section(std::string_view title) {
    const bool ansi = ansi_enabled();
    std::cout << (ansi ? cyan : "") << (ansi ? bold : "")
              << "\n  -- " << title << " " << (ansi ? reset : "") << "\n";
}

void ConsoleUi::success(std::string_view message) {
    std::cout << (ansi_enabled() ? green : "") << "  [OK] "
              << message << (ansi_enabled() ? reset : "") << '\n';
}

void ConsoleUi::error(std::string_view message) {
    std::cout << (ansi_enabled() ? red : "") << "  [ERROR] "
              << message << (ansi_enabled() ? reset : "") << '\n';
}

void ConsoleUi::info(std::string_view message) {
    std::cout << (ansi_enabled() ? yellow : "") << "  [INFO] "
              << message << (ansi_enabled() ? reset : "") << '\n';
}

bool ConsoleUi::read_number(std::string_view prompt, int& value) {
    std::cout << (ansi_enabled() ? cyan : "") << "\n  " << prompt << ": "
              << (ansi_enabled() ? reset : "");
    std::string input;
    if (!std::getline(std::cin, input)) return false;
    std::istringstream stream(input);
    return static_cast<bool>(stream >> value) &&
           stream.eof();
}

bool ConsoleUi::read_unsigned(std::string_view prompt, unsigned int& value) {
    std::cout << (ansi_enabled() ? cyan : "") << "\n  " << prompt << ": "
              << (ansi_enabled() ? reset : "");
    std::string input;
    if (!std::getline(std::cin, input)) return false;
    std::istringstream stream(input);
    return static_cast<bool>(stream >> value) &&
           stream.eof();
}

void ConsoleUi::startup() {
    std::cout << "  [STARTUP] C++17 | Standard library only | Mutex/CV/Semaphore\n"
              << "  [STARTUP] State=data/railsetu.state | Log=logs/activity.log\n"
              << "  [STARTUP] Invariant: confirmed seats <= capacity | FIFO priority waitlist\n";
}

void ConsoleUi::dashboard(std::string_view train_count,
                          std::string_view confirmed,
                          std::string_view waitlisted,
                          std::string_view bookings) {
    section("LIVE DASHBOARD");
    std::cout << "  +----------------+----------------+----------------+----------------+\n"
              << "  | TRAINS " << train_count << "      | CONFIRMED " << confirmed
              << " | WAITLIST " << waitlisted << "  | BOOKINGS " << bookings << " |\n"
              << "  +----------------+----------------+----------------+----------------+\n";
}

void ConsoleUi::footer() { std::cout << "  [READY] mutex=managed cv=predicate semaphore=armed persistence=atomic\n"; }
void ConsoleUi::suggestions() { std::cout << "  Suggestions: list | seats RS101 | book RS101 <name> 1 SENIOR | dashboard | report\n"; }

bool ConsoleUi::confirm(std::string_view prompt) {
    std::cout << "  CONFIRM: " << prompt << " [y/N]: ";
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

void ConsoleUi::route(std::string_view origin, std::string_view destination) {
    std::cout << "  ROUTE  " << origin << " ====[ RailSetu ]==== " << destination << "\n";
}

void ConsoleUi::occupancy(std::string_view label, std::size_t used, std::size_t capacity) {
    const std::size_t percent = capacity == 0 ? 0 : used * 100 / capacity;
    const std::size_t filled = percent / 10;
    std::cout << "  " << label << " [";
    for (std::size_t i = 0; i < 10; ++i) std::cout << (i < filled ? '#' : '-');
    std::cout << "] " << percent << "% (" << used << '/' << capacity << ")\n";
}

void ConsoleUi::seat_grid(std::string_view title,
                          const std::vector<std::string>& confirmed,
                          const std::vector<std::string>& reserved,
                          const std::vector<std::string>& promotion) {
    section(title);
    std::cout << "  Legend: [C] confirmed [R] reserved [P] promotion [.] available\n";
    for (std::size_t i = 0; i < confirmed.size(); ++i) {
        if (i % 6 == 0) std::cout << "  ";
        std::cout << "[C:" << confirmed[i] << "] ";
        if (i % 6 == 5) std::cout << '\n';
    }
    if (!confirmed.empty() && confirmed.size() % 6 != 0) std::cout << '\n';
    for (const std::string& seat : reserved) std::cout << "  [R:" << seat << "]";
    if (!reserved.empty()) std::cout << '\n';
    for (const std::string& seat : promotion) std::cout << "  [P:" << seat << "]";
    if (!promotion.empty()) std::cout << '\n';
}

void ConsoleUi::benchmark_chart(std::string_view title,
                                const std::vector<std::pair<std::string, std::size_t>>& bars) {
    section(title);
    std::size_t maximum = 1;
    for (const auto& bar : bars) maximum = bar.second > maximum ? bar.second : maximum;
    for (const auto& bar : bars) {
        const std::size_t length = bar.second * 40 / maximum;
        std::cout << "  " << bar.first << " |";
        for (std::size_t i = 0; i < length; ++i) std::cout << '#';
        std::cout << " " << bar.second << '\n';
    }
}

void ConsoleUi::ticket_card(const std::string& pnr,
                            const std::string& passenger,
                            const std::string& train,
                            const std::string& route_name,
                            const std::string& coach,
                            const std::vector<std::string>& seats,
                            const std::string& status,
                            const std::string& category,
                            std::uint64_t fare) {
    section("RAILSETU E-TICKET");
    std::cout << "  +==============================================================+\n"
              << "  | PNR  " << pnr << "                                  RAILSETU |\n"
              << "  +--------------------------------------------------------------+\n"
              << "  | Passenger : " << passenger << '\n'
              << "  | Train     : " << train << '\n'
              << "  | Route     : " << route_name << '\n'
              << "  | Coach     : " << (coach.empty() ? "-" : coach) << '\n'
              << "  | Seats     : ";
    if (seats.empty()) std::cout << "-";
    for (std::size_t i = 0; i < seats.size(); ++i) {
        if (i) std::cout << ", ";
        std::cout << seats[i];
    }
    std::cout << "\n  | Category  : " << category
              << "\n  | Status    : " << status
              << "\n  | Fare      : INR " << fare
              << "\n  +==============================================================+\n";
}

void ConsoleUi::waitlist_ladder(const std::vector<std::pair<std::string, std::string>>& entries) {
    section("WAITLIST PRIORITY LADDER");
    if (entries.empty()) { std::cout << "  [ EMPTY ]\n"; return; }
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const char* color = ansi_enabled() ?
            (entries[i].second == std::string("SENIOR") ? "\033[35m" :
             entries[i].second == std::string("LADIES") ? "\033[36m" :
             entries[i].second == std::string("TATKAL") ? "\033[33m" : "\033[37m") : "";
        std::cout << "  #" << i + 1 << " " << color << "[" << entries[i].second << "] "
                  << entries[i].first << (ansi_enabled() ? reset : "") << '\n';
    }
}

void ConsoleUi::activity_feed(const std::vector<std::string>& entries) {
    section("LIVE ACTIVITY FEED");
    if (entries.empty()) std::cout << "  No recent activity\n";
    else for (const std::string& entry : entries) std::cout << "  > " << entry << '\n';
}

void ConsoleUi::booking_progress(std::string_view action) {
    std::cout << "  " << action << " ... complete\n";
}

}
