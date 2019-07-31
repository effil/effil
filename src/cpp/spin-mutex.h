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

    /// Lua API methods
    void luaUniqueLock(const sol::stack_object& clbk);
    void luaSharedLock(const sol::stack_object& clbk);
    bool luaTryUniqueLock(const sol::stack_object& clbk);
    bool luaTrySharedLock(const sol::stack_object& objMutex);

    static void exportAPI(sol::state_view& lua);

private:
    std::atomic_int counter_ {0};
    std::atomic_bool lock_ {false};
};

} // effil
