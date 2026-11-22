#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

namespace railsetu {

class Logger {
public:
    explicit Logger(std::string file);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const std::string& level, const std::string& message);
    void log(const std::string& level,
             const std::string& event,
             const std::unordered_map<std::string, std::string>& fields);

private:
    std::string file_;
    std::mutex mutex_;
};

} // namespace railsetu
