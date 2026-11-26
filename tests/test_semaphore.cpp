#include "../include/railsetu/Semaphore.hpp"
#include <cassert>
#include <thread>

int main() {
    railsetu::Semaphore semaphore(0);
    std::thread releaser([&semaphore] { semaphore.release(); });
    semaphore.acquire();
    releaser.join();
    assert(!semaphore.try_acquire());
    return 0;
}
