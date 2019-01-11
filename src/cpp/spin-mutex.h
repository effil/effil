#pragma once

#include "sol.hpp"

#include <atomic>
#include <thread>

namespace effil {

class SpinMutex {
public:
    void lock() noexcept;
    bool try_lock() noexcept;
    void unlock() noexcept;

    void lock_shared() noexcept;
    bool try_lock_shared() noexcept;
    void unlock_shared() noexcept;

private:
    std::atomic_int counter_ {0};
    std::atomic_bool lock_ {false};
};

namespace mutex {

void uniqueLock(const sol::stack_object& objMutex, const sol::stack_object& objFunc);
void sharedLock(const sol::stack_object& objMutex, const sol::stack_object& objFunc);

bool tryUniqueLock(const sol::stack_object& objMutex,
                   const sol::stack_object& objFunc);
bool trySharedLock(const sol::stack_object& objMutex,
                   const sol::stack_object& objFunc);

} // mutex

} // effil
