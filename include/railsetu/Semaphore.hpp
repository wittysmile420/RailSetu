#pragma once

#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace railsetu {

class Semaphore {
public:
    explicit Semaphore(std::size_t count = 0) : count_(count) {}

    void release(std::size_t n = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (n > std::numeric_limits<std::size_t>::max() - count_) {
            throw std::overflow_error("semaphore permit count overflow");
        }
        count_ += n;
        cv_.notify_all();
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return count_ > 0; });
        --count_;
    }

    bool try_acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ == 0) return false;
        --count_;
        return true;
    }

    std::size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::size_t count_;
};

} // namespace railsetu
