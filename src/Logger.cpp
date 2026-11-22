#include "railsetu/Logger.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>
#include <algorithm>
#include <vector>

namespace railsetu {

Logger::Logger(std::string file) : file_(std::move(file)) {}
Logger::~Logger() = default;

void Logger::log(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::filesystem::path path(file_);
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(file_, std::ios::app);
    if (!out) return;
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&tm, &time);
#endif
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " "
        << "[" << level << "] " << message << '\n';
}

void Logger::log(const std::string& level,
                 const std::string& event,
                 const std::unordered_map<std::string, std::string>& fields) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::filesystem::path path(file_);
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(file_, std::ios::app);
    if (!out) return;
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&tm, &time);
#endif
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << " "
        << "{\"level\":\"" << level << "\",\"event\":\"" << event << "\"";
    std::vector<std::pair<std::string, std::string>> sorted(fields.begin(), fields.end());
    std::sort(sorted.begin(), sorted.end());
    for (const auto& field : sorted)
        out << ",\"" << field.first << "\":\"" << field.second << "\"";
    out << "}\n";
}

} // namespace railsetu
