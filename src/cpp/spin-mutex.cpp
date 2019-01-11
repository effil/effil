#include "spin-mutex.h"
#include "lua-helpers.h"

namespace effil {

void SpinMutex::lock() noexcept {
    while (lock_.exchange(true, std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (counter_ != 0) {
        std::this_thread::yield();
    }
}

bool SpinMutex::try_lock() noexcept {
    if (lock_.exchange(true, std::memory_order_acquire)) {
        return false;
    }
    if (counter_ != 0) {
        lock_.exchange(false, std::memory_order_release);
        return false;
    }
    return true;
}

void SpinMutex::unlock() noexcept {
    lock_.exchange(false, std::memory_order_release);
}

void SpinMutex::lock_shared() noexcept {
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

bool SpinMutex::try_lock_shared() noexcept {
    if (lock_) {
        return false;
    }

    counter_.fetch_add(1, std::memory_order_acquire);

    if (lock_) {
        counter_.fetch_sub(1, std::memory_order_release);
        return false;
    }
    return true;
}

void SpinMutex::unlock_shared() noexcept {
    counter_.fetch_sub(1, std::memory_order_release);
}

namespace mutex {

void uniqueLock(const sol::stack_object& objMutex,
                const sol::stack_object& objFunc) {
    REQUIRE(objMutex.is<SpinMutex>())
            << "bad argument #1 to 'effil.unique_lock' (mutex expected, got "
            << luaTypename(objMutex) << ")";
    REQUIRE(objFunc.get_type() == sol::type::function)
            << "bad argument #2 to 'effil.unique_lock' (function expected, got "
            << luaTypename(objFunc) << ")";
    auto& mutex = objMutex.as<SpinMutex>();
    const auto& func = objFunc.as<sol::protected_function>();

    mutex.lock();
    ScopeGuard g([&mutex](){ mutex.unlock(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
}

void sharedLock(const sol::stack_object& objMutex,
                const sol::stack_object& objFunc) {
    REQUIRE(objMutex.is<SpinMutex>())
            << "bad argument #1 to 'effil.shared_lock' (mutex expected, got "
            << luaTypename(objMutex) << ")";
    REQUIRE(objFunc.get_type() == sol::type::function)
            << "bad argument #2 to 'effil.shared_lock' (function expected, got "
            << luaTypename(objFunc) << ")";
    auto& mutex = objMutex.as<SpinMutex>();
    const auto& func = objFunc.as<sol::function>();

    mutex.lock_shared();
    ScopeGuard g([&mutex](){ mutex.unlock_shared(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
}

bool tryUniqueLock(const sol::stack_object& objMutex,
                   const sol::stack_object& objFunc) {
    REQUIRE(objMutex.is<SpinMutex>())
            << "bad argument #1 to 'effil.try_unique_lock' (mutex expected, got "
            << luaTypename(objMutex) << ")";
    REQUIRE(objFunc.get_type() == sol::type::function)
            << "bad argument #2 to 'effil.try_unique_lock' (function expected, got "
            << luaTypename(objFunc) << ")";
    auto& mutex = objMutex.as<SpinMutex>();
    const auto& func = objFunc.as<sol::function>();

    if (!mutex.try_lock())
        return false;

    ScopeGuard g([&mutex](){ mutex.unlock(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
    return true;
}

bool trySharedLock(const sol::stack_object& objMutex,
                   const sol::stack_object& objFunc) {
    REQUIRE(objMutex.is<SpinMutex>())
            << "bad argument #1 to 'effil.try_shared_lock' (mutex expected, got "
            << luaTypename(objMutex) << ")";
    REQUIRE(objFunc.get_type() == sol::type::function)
            << "bad argument #2 to 'effil.try_shared_lock' (function expected, got "
            << luaTypename(objFunc) << ")";
    auto& mutex = objMutex.as<SpinMutex>();
    const auto& func = objFunc.as<sol::function>();

    if (!mutex.try_lock_shared())
        return false;

    ScopeGuard g([&mutex](){ mutex.unlock_shared(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
    return true;
}

} // mutex

} // effil
