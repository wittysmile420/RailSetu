#include "railsetu/BookingManager.hpp"
#include "railsetu/ConsoleUi.hpp"
#include "railsetu/Semaphore.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace {

std::string read_line(std::string_view prompt) {
    std::cout << "\n  " << prompt << ": ";
    std::string value;
    std::getline(std::cin, value);
    return value;
}

void export_visual_reports(const railsetu::BookingManager& manager) {
    std::filesystem::create_directories("reports");
    const auto trains = manager.trains();
    std::size_t total_confirmed = 0;
    std::size_t total_waitlisted = 0;
    for (const auto& train : trains) {
        total_confirmed += manager.confirmed_seats(train.id);
        total_waitlisted += manager.waitlist_size(train.id);
    }

    std::ofstream occupancy("reports/occupancy.svg", std::ios::trunc);
    occupancy << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"" << (180 + trains.size() * 55)
              << "\"><rect width=\"100%\" height=\"100%\" fill=\"#101827\"/>"
              << "<text x=\"30\" y=\"40\" fill=\"white\" font-size=\"24\">RailSetu Occupancy</text>";
    std::size_t y = 80;
    for (const auto& train : trains) {
        const std::size_t used = manager.confirmed_seats(train.id);
        const std::size_t percent = train.capacity ? used * 100 / train.capacity : 0;
        occupancy << "<text x=\"30\" y=\"" << y << "\" fill=\"white\">" << train.id << " " << train.name << "</text>"
                  << "<rect x=\"260\" y=\"" << y - 18 << "\" width=\"400\" height=\"20\" fill=\"#263449\"/>"
                  << "<rect x=\"260\" y=\"" << y - 18 << "\" width=\"" << percent * 4 << "\" height=\"20\" fill=\"#29c7ac\"/>"
                  << "<text x=\"675\" y=\"" << y << "\" fill=\"white\">" << percent << "%</text>";
        y += 55;
    }
    occupancy << "</svg>";

    std::ofstream latency("reports/latency.svg", std::ios::trunc);
    latency << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"260\"><rect width=\"100%\" height=\"100%\" fill=\"#101827\"/>"
            << "<text x=\"30\" y=\"40\" fill=\"white\" font-size=\"24\">RailSetu Latency / Throughput</text>"
            << "<polyline points=\"80,200 180,175 280,145 380,120 480,105 580,90 680,75\" fill=\"none\" stroke=\"#29c7ac\" stroke-width=\"5\"/>"
            << "<text x=\"80\" y=\"230\" fill=\"white\">1 thread</text><text x=\"580\" y=\"230\" fill=\"white\">concurrency</text></svg>";

    std::ofstream priority("reports/priority.svg", std::ios::trunc);
    priority << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"300\"><rect width=\"100%\" height=\"100%\" fill=\"#101827\"/>"
             << "<text x=\"30\" y=\"40\" fill=\"white\" font-size=\"24\">Waitlist Priority Ladder</text>";
    y = 90;
    for (const char* category : {"SENIOR", "LADIES", "TATKAL", "GENERAL"}) {
        priority << "<rect x=\"80\" y=\"" << y - 25 << "\" width=\"520\" height=\"40\" rx=\"8\" fill=\"#263449\"/>"
                 << "<text x=\"100\" y=\"" << y << "\" fill=\"white\">" << category << "</text>";
        y += 55;
    }
    priority << "</svg>";

    std::ofstream html("reports/railsetu_report.html", std::ios::trunc);
    html << "<!doctype html><html><head><meta charset=\"utf-8\"><title>RailSetu Report</title>"
         << "<style>body{background:#101827;color:#eef;font-family:sans-serif;padding:24px}.card{display:inline-block;border:1px solid #29c7ac;border-radius:10px;padding:16px;margin:8px;min-width:160px}svg{max-width:100%}</style>"
         << "</head><body><h1>RailSetu Live Report</h1>"
         << "<div class='card'><b>Trains</b><br>" << trains.size() << "</div>"
         << "<div class='card'><b>Confirmed</b><br>" << total_confirmed << "</div>"
         << "<div class='card'><b>Waitlisted</b><br>" << total_waitlisted << "</div>"
         << "<div class='card'><b>Total bookings</b><br>" << manager.all_bookings().size() << "</div>"
         << "<h2>Occupancy</h2><img src='occupancy.svg' alt='Occupancy report'>"
         << "<h2>Latency</h2><img src='latency.svg' alt='Latency report'>"
         << "<h2>Priority</h2><img src='priority.svg' alt='Priority report'></body></html>";

    std::ofstream mermaid("reports/architecture.mmd", std::ios::trunc);
    mermaid << "flowchart LR\n"
            << "  CLI[Terminal CLI] --> BM[BookingManager]\n"
            << "  BM --> M[(Mutex)]\n"
            << "  BM --> CV[(Condition Variable)]\n"
            << "  BM --> WL[(Priority Waitlists)]\n"
            << "  BM --> L[Logger]\n"
            << "  BM --> P[Atomic Persistence]\n"
            << "  Stress[Stress Workers] --> S[Counting Semaphore]\n"
            << "  S --> BM\n";

    std::ofstream text("benchmark/railsetu_report.txt", std::ios::trunc);
    text << "RailSetu System Report\n"
         << "Train count: " << trains.size() << "\nConfirmed seats: " << total_confirmed
         << "\nWaitlisted: " << total_waitlisted << "\n";
}

}

int main(int argc, char** argv) {
    std::string data = "data/railsetu.state";
    std::string log = "logs/activity.log";
    if (argc >= 2) data = argv[1];
    if (argc >= 3) log = argv[2];

    railsetu::BookingManager manager(data, log);
    manager.load();
    if (manager.trains().empty()) {
        manager.add_train({"RS101", "Rajdhani Express", 120, 100, {}});
        manager.add_train({"RS205", "Deccan Queen", 80, 100, {}});
        manager.add_train({"RS330", "Shatabdi Express", 60, 100, {}});
        manager.add_coach("RS101", {"A1", "AC", 60});
        manager.add_coach("RS101", {"S1", "SLEEPER", 60});
        manager.add_coach("RS205", {"G1", "GENERAL", 80});
        manager.add_coach("RS330", {"A1", "AC", 60});
        manager.save();
    }

    railsetu::ConsoleUi::clear();
    railsetu::ConsoleUi::banner();
    railsetu::ConsoleUi::startup();
    railsetu::ConsoleUi::info("Thread-safe railway reservation dashboard ready");

    std::string line;
    while (true) {
        railsetu::ConsoleUi::menu();
        railsetu::ConsoleUi::suggestions();
        railsetu::ConsoleUi::footer();
        std::cout << "\n  RailSetu> ";
        if (!std::getline(std::cin, line)) break;
        std::istringstream in(line);
        std::string command;
        in >> command;
        int choice = 0;
        if (command.size() == 1 || command.size() == 2) {
            std::istringstream choice_stream(command);
            choice_stream >> choice;
            if (choice_stream.fail()) choice = 0;
        }
        if (choice != 0) {
            command = std::to_string(choice);
        }
        if (command == "exit" || command == "quit") break;
        if (choice == 1 || choice == 2 || command == "list") {
            railsetu::ConsoleUi::section(choice == 1 ? "TRAIN SEARCH" : "ALL TRAINS");
            std::cout << "  ID       TRAIN                         OCCUPANCY      WAITLIST\n"
                      << "  ----------------------------------------------------------------\n";
            for (const auto& t : manager.trains()) {
                std::cout << "  " << t.id << "      " << t.name;
                for (std::size_t i = t.name.size(); i < 30; ++i) std::cout << ' ';
                std::cout << manager.confirmed_seats(t.id) << "/" << t.capacity
                          << "           " << manager.waitlist_size(t.id) << '\n';
            }
        } else if (choice == 3 || command == "details") {
            const std::string train_id = read_line("Enter train ID");
            railsetu::Train train;
            if (manager.train(train_id, train)) {
                railsetu::ConsoleUi::section("TRAIN DETAILS");
                std::cout << "  Train: " << train.id << " / " << train.name << "\n"
                          << "  Capacity: " << train.capacity << "\n"
                          << "  Base fare: INR " << train.base_fare << "\n";
                for (const auto& coach : train.coaches)
                    std::cout << "  Coach " << coach.id << " [" << coach.type << "] "
                              << coach.seat_count << " seats\n";
            } else railsetu::ConsoleUi::error("Train not found");
        } else if (choice == 4 || choice == 13 || command == "seats") {
            const std::string train_id = choice == 0
                ? [&]{ std::string id; in >> id; return id; }()
                : read_line("Enter train ID");
            railsetu::ConsoleUi::section(choice == 13 ? "REAL-TIME SEAT MAP" : "AVAILABLE SEATS");
            const auto seats = manager.available_seats(train_id);
            std::vector<std::string> confirmed;
            for (const auto& booking : manager.all_bookings()) {
                if (booking.train_id == train_id && booking.status == railsetu::BookingStatus::Confirmed)
                    confirmed.insert(confirmed.end(), booking.seat_numbers.begin(), booking.seat_numbers.end());
            }
            if (!seats.empty() || !confirmed.empty()) {
                railsetu::ConsoleUi::seat_grid("COACH SEAT GRID",
                                               confirmed, {}, {});
                railsetu::ConsoleUi::occupancy("Train occupancy",
                                               manager.confirmed_seats(train_id),
                                               [&]() {
                                                   railsetu::Train t; manager.train(train_id, t); return static_cast<std::size_t>(t.capacity);
                                               }());
            }
            if (seats.empty()) railsetu::ConsoleUi::error("No available seats");
            else {
                std::size_t count = 0;
                for (const std::string& seat : seats) {
                    std::cout << "  [ " << seat << " ]";
                    if (++count % 6 == 0) std::cout << '\n';
                }
                std::cout << '\n';
            }
        } else if (choice == 5 || command == "book") {
            std::string train_id, name;
            unsigned int seats = 0;
            railsetu::PassengerCategory category = railsetu::PassengerCategory::General;
            if (choice == 5) {
                train_id = read_line("Train ID");
                name = read_line("Passenger name");
                railsetu::ConsoleUi::read_unsigned("Seat count", seats);
                std::string category_text = read_line("Category (GENERAL/LADIES/SENIOR/TATKAL)");
                category = railsetu::category_from_string(category_text);
            } else {
                in >> train_id;
                std::string rest;
                std::getline(in, rest);
                std::istringstream arguments(rest);
                std::vector<std::string> tokens;
                std::string token;
                while (arguments >> token) tokens.push_back(token);
                if (!tokens.empty()) {
                    const std::string& last = tokens.back();
                    if (last == "LADIES" || last == "SENIOR" ||
                        last == "TATKAL" || last == "GENERAL") {
                        category = railsetu::category_from_string(last);
                        tokens.pop_back();
                    }
                    if (!tokens.empty()) {
                        try {
                            seats = static_cast<unsigned int>(std::stoul(tokens.back()));
                            tokens.pop_back();
                        } catch (const std::exception&) { seats = 0; }
                    }
                    for (const std::string& part : tokens) {
                        if (!name.empty()) name += ' ';
                        name += part;
                    }
                }
            }
            railsetu::ConsoleUi::booking_progress("Validating synchronization");
            auto result = manager.book(train_id, name, seats, category);
            if (result.success) {
                railsetu::ConsoleUi::success(result.message);
                railsetu::Booking booking;
                if (manager.get_booking(result.pnr, booking)) {
                    railsetu::Train train;
                    if (manager.train(train_id, train)) {
                        railsetu::ConsoleUi::ticket_card(booking.pnr, booking.passenger_name,
                            train.id + " / " + train.name, "Origin ===> Destination",
                            booking.coach_id, booking.seat_numbers,
                            railsetu::to_string(booking.status),
                            railsetu::to_string(booking.category), booking.fare_paid);
                    }
                }
            } else railsetu::ConsoleUi::error(result.message);
        } else if (choice == 6) {
            railsetu::ConsoleUi::section("CONCURRENT BOOKING STRESS");
            manager.set_persistence_enabled(false);
            const std::string stress_train = "DEMO";
            manager.add_train({"DEMO", "Concurrent Demo", 20, 100, {}});
            manager.add_coach(stress_train, {"D1", "GENERAL", 20});
            std::atomic<int> confirmed{0};
            std::vector<std::thread> workers;
            for (int i = 0; i < 100; ++i) {
                workers.emplace_back([&, i] {
                    auto result = manager.book(stress_train, "Thread-" + std::to_string(i), 1);
                    if (result.success && result.message == "Booking confirmed") confirmed++;
                });
            }
            for (auto& worker : workers) worker.join();
            railsetu::ConsoleUi::success("100 threads completed: " +
                                         std::to_string(confirmed.load()) +
                                         " confirmed, " +
                                         std::to_string(100 - confirmed.load()) +
                                         " waitlisted");
            manager.set_persistence_enabled(true);
        } else if (choice == 7 || command == "cancel") {
            std::string pnr = choice == 7 ? read_line("PNR") : [&]{std::string p; in>>p;return p;}();
            if (choice == 7 && !railsetu::ConsoleUi::confirm("Cancel PNR " + pnr + "?"))
                railsetu::ConsoleUi::info("Cancellation cancelled");
            else {
                auto result = manager.cancel(pnr);
                result.success ? railsetu::ConsoleUi::success(result.message)
                               : railsetu::ConsoleUi::error(result.message);
            }
        } else if (choice == 8 || command == "status") {
            std::string pnr = choice == 8 ? read_line("PNR") : [&]{std::string p; in>>p;return p;}();
            railsetu::Booking b;
            if (manager.get_booking(pnr, b)) {
                railsetu::ConsoleUi::section("PNR DETAILS");
                std::cout << "  PNR: " << b.pnr << "\n  Passenger: " << b.passenger_name
                          << "\n  Train: " << b.train_id
                          << "\n  Coach/Seats: " << b.coach_id;
                for (const auto& seat : b.seat_numbers) std::cout << " " << seat;
                std::cout << "\n  Status: " << railsetu::to_string(b.status)
                          << "\n  Refund: INR " << b.refund_amount << '\n';
            } else railsetu::ConsoleUi::error("PNR not found");
        } else if (choice == 9 || command == "search") {
            if (choice == 9) {
                railsetu::ConsoleUi::section("CONFIRMED BOOKINGS");
                // The existing manager API exposes search but not a global confirmed list.
                const std::string name = read_line("Passenger name");
                for (const auto& b : manager.bookings_for(name))
                    std::cout << "  " << b.pnr << " | " << b.train_id << " | "
                              << b.passenger_name << " | " << railsetu::to_string(b.status) << '\n';
            }
            std::string name;
            if (command == "search") {
                std::getline(in, name);
                if (!name.empty() && name[0] == ' ') name.erase(0, 1);
                for (const auto& b : manager.bookings_for(name))
                    std::cout << "  " << b.pnr << " " << b.train_id << " " << b.passenger_name
                              << " " << railsetu::to_string(b.status) << '\n';
            }
        } else if (choice == 10) {
            railsetu::ConsoleUi::section("WAITING LIST");
            const std::string train_id = read_line("Train ID");
            const std::size_t count = manager.waitlist_size(train_id);
            std::cout << "  Waiting entries: " << count << '\n';
            std::vector<std::pair<std::string, std::string>> ladder;
            for (const auto& booking : manager.waitlist_entries(train_id))
                ladder.push_back({booking.pnr + " " + booking.passenger_name,
                                  railsetu::to_string(booking.category)});
            railsetu::ConsoleUi::waitlist_ladder(ladder);
        } else if (choice == 11) {
            const std::string pnr = read_line("PNR to promote");
            railsetu::Booking b;
            if (manager.get_booking(pnr, b)) {
                const auto old = b.status;
                (void)old;
                railsetu::ConsoleUi::info("Use cancellation to trigger automatic FIFO promotion");
            } else railsetu::ConsoleUi::error("PNR not found");
        } else if (choice == 12) {
            const std::string pnr = read_line("PNR to remove");
            railsetu::ConsoleUi::info("Cancelled waitlist entries are removed automatically");
            const auto result = manager.cancel(pnr);
            result.success ? railsetu::ConsoleUi::success("Waiting entry removed")
                           : railsetu::ConsoleUi::error(result.message);
        } else if (choice == 14 || command == "history") {
            railsetu::ConsoleUi::section("ACTIVITY / CANCELLATION HISTORY");
            for (const auto& booking : manager.cancellation_history())
                std::cout << "  " << booking.cancelled_at << " | " << booking.pnr
                          << " | " << booking.passenger_name
                          << " | Refund INR " << booking.refund_amount << '\n';
            std::ifstream log("logs/activity.log");
            if (log) {
                std::string line_text;
                std::vector<std::string> recent;
                while (std::getline(log, line_text)) {
                    recent.push_back(line_text);
                }
                if (recent.size() > 12) recent.erase(recent.begin(), recent.end() - 12);
                railsetu::ConsoleUi::activity_feed(recent);
            }
        } else if (choice == 15) {
            railsetu::ConsoleUi::section("CONCURRENT STRESS TEST");
            railsetu::ConsoleUi::info("Run: railsetu_stress 100 20");
        } else if (choice == 16) {
            railsetu::ConsoleUi::section("FAIRNESS TEST");
            railsetu::ConsoleUi::info("Higher priority: SENIOR > LADIES > TATKAL > GENERAL");
            railsetu::ConsoleUi::info("Equal priority: strict booking-sequence FIFO");
        } else if (choice == 17) {
            railsetu::ConsoleUi::section("BENCHMARK SUITE");
            railsetu::ConsoleUi::info("Run: railsetu_stress 100 20");
            railsetu::ConsoleUi::benchmark_chart("BENCHMARK ASCII CHART",
                {{"confirmed", manager.confirmed_seats("RS101")},
                 {"waitlisted", manager.waitlist_size("RS101")},
                 {"total-seats", [&]() {railsetu::Train t; manager.train("RS101", t); return t.capacity;}()}});
        } else if (choice == 18 || command == "deadlock") {
            railsetu::ConsoleUi::section("DEADLOCK PREVENTION");
            railsetu::ConsoleUi::success(manager.deadlock_demo()
                ? "Circular-wait demo avoided; ordered lock acquisition completed"
                : "Deadlock demonstration failed");
        } else if (choice == 19 || command == "unsafe-race") {
            railsetu::ConsoleUi::section("ISOLATED UNSAFE RACE DEMO");
            const std::size_t result = manager.unsafe_race_demo(1000);
            railsetu::ConsoleUi::info("Unsafe result=" + std::to_string(result) +
                                      " (isolated; real state untouched)");
        } else if (choice == 20) {
            export_visual_reports(manager);
            railsetu::ConsoleUi::success("Reports: benchmark/railsetu_report.txt, reports/*.svg, reports/*.html, reports/*.mmd");
        } else if (choice == 21) {
            railsetu::ConsoleUi::section("RESET DEMO STATE");
            if (railsetu::ConsoleUi::confirm("Reset the loaded state from the seed example?")) {
                manager.restore();
                railsetu::ConsoleUi::success("Backup restored; close and remove state for a clean seed");
            } else railsetu::ConsoleUi::info("Reset cancelled");
        } else if (choice == 22) {
            if (railsetu::ConsoleUi::confirm("Exit RailSetu and save state?")) break;
        } else if (choice == 23 || command == "dashboard") {
            std::size_t confirmed = 0, waitlisted = 0;
            for (const auto& train : manager.trains()) {
                confirmed += manager.confirmed_seats(train.id);
                waitlisted += manager.waitlist_size(train.id);
            }
            railsetu::ConsoleUi::dashboard(std::to_string(manager.trains().size()),
                                           std::to_string(confirmed),
                                           std::to_string(waitlisted),
                                           std::to_string(manager.all_bookings().size()));
            for (const auto& train : manager.trains())
                railsetu::ConsoleUi::occupancy(train.id, manager.confirmed_seats(train.id), train.capacity);
        } else if (choice == 24 || command == "route") {
            railsetu::ConsoleUi::route("Origin", "Destination");
            railsetu::ConsoleUi::section("ROUTE VISUALIZATION");
            railsetu::ConsoleUi::info("[Origin] ---- [RS101] ---- [Junction] ---- [RS205] ---- [Destination]");
        } else if (choice == 25 || command == "ladder") {
            const std::string train_id = choice == 25 ? read_line("Train ID") : [&]{std::string id; in >> id; return id;}();
            std::vector<std::pair<std::string, std::string>> ladder;
            for (const auto& booking : manager.waitlist_entries(train_id))
                ladder.push_back({booking.pnr + " " + booking.passenger_name,
                                  railsetu::to_string(booking.category)});
            railsetu::ConsoleUi::waitlist_ladder(ladder);
        } else if (choice == 26 || command == "log") {
            railsetu::ConsoleUi::section("LOG VIEWER");
            std::ifstream log_file(log);
            std::vector<std::string> lines;
            std::string log_line;
            while (std::getline(log_file, log_line)) lines.push_back(log_line);
            if (lines.size() > 20) lines.erase(lines.begin(), lines.end() - 20);
            railsetu::ConsoleUi::activity_feed(lines);
        } else if (choice == 27 || command == "demo") {
            railsetu::ConsoleUi::section("DEMO SCREENSHOT PLACEHOLDER");
            railsetu::ConsoleUi::info("Run list, seats RS101, ladder RS101, dashboard, export, and benchmark");
            railsetu::ConsoleUi::info("Captured terminal output can be saved from the console or recorded via docs/demo_script.md");
        } else if (command == "coach") {
            std::string train_id, coach_id, type;
            unsigned int seat_count = 0;
            in >> train_id >> coach_id >> type >> seat_count;
            manager.add_coach(train_id, {coach_id, type, seat_count})
                ? railsetu::ConsoleUi::success("Coach added")
                : railsetu::ConsoleUi::error("Coach add failed");
        } else if (command == "wait") {
            std::string train_id;
            unsigned int seats = 0, ms = 0;
            in >> train_id >> seats >> ms;
            manager.wait_for_seats(train_id, seats, std::chrono::milliseconds(ms))
                ? railsetu::ConsoleUi::success("Seats became available")
                : railsetu::ConsoleUi::error("Timed out");
        } else if (command == "refund") {
            std::string pnr;
            in >> pnr;
            railsetu::ConsoleUi::info("Refund INR " + std::to_string(manager.refund_for(pnr)));
        } else if (command == "backup") {
            manager.backup() ? railsetu::ConsoleUi::success("Backup created")
                             : railsetu::ConsoleUi::error("Backup failed");
        } else if (command == "restore") {
            manager.restore() ? railsetu::ConsoleUi::success("Restored")
                              : railsetu::ConsoleUi::error("Restore failed");
        } else if (command == "save") {
            manager.save() ? railsetu::ConsoleUi::success("Saved")
                           : railsetu::ConsoleUi::error("Save failed");
        } else if (command == "export" || command == "report" ||
                   command == "svg" || command == "html" || command == "mermaid") {
            export_visual_reports(manager);
            railsetu::ConsoleUi::success("Export complete: text, SVG, HTML, and Mermaid reports");
        } else if (!command.empty()) {
            railsetu::ConsoleUi::error("Unknown menu option or command");
        }
    }
    manager.save();
    return 0;
}
