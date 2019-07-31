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

void SpinMutex::luaUniqueLock(const sol::stack_object& clbk) {
    REQUIRE(clbk.get_type() == sol::type::function)
            << "bad argument #1 to 'mutex.unique_lock' (function expected, got "
            << luaTypename(clbk) << ")";

    const auto& func = clbk.as<sol::protected_function>();

    lock();
    ScopeGuard g([this](){ unlock(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
}

void SpinMutex::luaSharedLock(const sol::stack_object& clbk) {
    REQUIRE(clbk.get_type() == sol::type::function)
            << "bad argument #1 to 'mutex.shared_lock' (function expected, got "
            << luaTypename(clbk) << ")";

    const auto& func = clbk.as<sol::protected_function>();

    lock_shared();
    ScopeGuard g([this](){ unlock_shared(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();

}

bool SpinMutex::luaTryUniqueLock(const sol::stack_object& clbk) {
    REQUIRE(clbk.get_type() == sol::type::function)
            << "bad argument #1 to 'mutex.try_unique_lock' (function expected, got "
            << luaTypename(clbk) << ")";

    const auto& func = clbk.as<sol::protected_function>();

    if (!try_lock())
        return false;

    ScopeGuard g([this](){ unlock(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
    return true;
}

bool SpinMutex::luaTrySharedLock(const sol::stack_object& clbk) {
    REQUIRE(clbk.get_type() == sol::type::function)
            << "bad argument #1 to 'mutex.try_shared_lock' (function expected, got "
            << luaTypename(clbk) << ")";

    const auto& func = clbk.as<sol::protected_function>();

    if (!try_lock_shared())
        return false;

    ScopeGuard g([this](){ unlock_shared(); });
    const auto ret = func();
    if (!ret.valid())
        throw ret.get<sol::error>();
    return true;
}

void SpinMutex::exportAPI(sol::state_view& lua) {
    sol::usertype<SpinMutex> type("new", sol::no_constructor,
        "unique_lock",  &SpinMutex::luaUniqueLock,
        "shared_lock",  &SpinMutex::luaSharedLock,
        "try_unique_lock", &SpinMutex::luaTryUniqueLock,
        "try_shared_lock", &SpinMutex::luaTrySharedLock
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

} // effil
