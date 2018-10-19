#pragma once

#include <this_thread.h>
#include <lua-helpers.h>

#include <mutex>
#include <condition_variable>

namespace effil {

struct IInterruptable {
    virtual void interrupt() = 0;
};

class Notifier : public IInterruptable {
public:
    typedef std::function<void()> InterruptChecker;

    Notifier() : notified_(false) {}

    void notify() {
        std::unique_lock<std::mutex> lock(mutex_);
        notified_ = true;
        cv_.notify_all();
    }

    void interrupt() final {
        cv_.notify_all();
    }

    void wait() {
        this_thread::interruptionPoint();

        this_thread::ScopedSetInterruptable interruptable(this);

        std::unique_lock<std::mutex> lock(mutex_);
        while (!notified_) {
            cv_.wait(lock);
            this_thread::interruptionPoint();
        }
    }

    template <typename T>
    bool waitFor(T period) {
        this_thread::interruptionPoint();

        if (period == std::chrono::seconds(0) || notified_)
            return notified_;

        this_thread::ScopedSetInterruptable interruptable(this);

        Timer timer(period);
        std::unique_lock<std::mutex> lock(mutex_);
        while (!timer.isFinished() &&
               cv_.wait_for(lock, timer.left()) != std::cv_status::timeout &&
               !notified_) {
            this_thread::interruptionPoint();
        }
        return notified_;
    }

    void reset() {
        notified_ = false;
    }

private:
    bool notified_;
    std::mutex mutex_;
    std::condition_variable cv_;

private:
    Notifier(Notifier& ) = delete;
};

} // namespace effil
