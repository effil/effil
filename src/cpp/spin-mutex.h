#pragma once

#include <atomic>
#include <thread>

namespace effil {

class SpinMutex {
public:
    void lock() noexcept {
        while (lock_.exchange(true, std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        while (counter_ != 0) {
            std::this_thread::yield();
        }
    }

    void unlock() noexcept {
        lock_.exchange(false, std::memory_order_release);
    }

    void lock_shared() noexcept {
        while (true) {
            while (lock_) {
                std::this_thread::yield();
            }

            counter_.fetch_add(1, std::memory_order_acquire);

            if (lock_)
                counter_.fetch_sub(1, std::memory_order_release);
            else
                return;
        }
    }

    void unlock_shared() noexcept {
        counter_.fetch_sub(1, std::memory_order_release);
    }

private:
    std::atomic_int counter_ {0};
    std::atomic_bool lock_ {false};
};

} // effil
