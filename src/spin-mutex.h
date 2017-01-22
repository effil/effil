#pragma once

#include <atomic>
#include <thread>

namespace effil {

class SpinMutex {
public:
    void lock() noexcept {
        while(lock_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void unlock() noexcept {
        lock_.clear(std::memory_order_release);
    }

private:
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};

} // effil
