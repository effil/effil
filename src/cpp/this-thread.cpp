#include "this-thread.h"

#include "thread-handle.h"
#include "notifier.h"

namespace effil {
namespace this_thread {

ScopedSetInterruptable::ScopedSetInterruptable(IInterruptable* notifier) {
    if (const auto thisThread = ThreadHandle::getThis()) {
        thisThread->setNotifier(notifier);
    }
}

ScopedSetInterruptable::~ScopedSetInterruptable() {
    if (const auto thisThread = ThreadHandle::getThis()) {
        thisThread->setNotifier(nullptr);
    }
}

void cancellationPoint() {
    const auto thisThread = ThreadHandle::getThis();
    if (thisThread && thisThread->command() == ThreadHandle::Command::Cancel) {
        thisThread->changeStatus(ThreadHandle::Status::Cancelled);
        throw ThreadCancelException();
    }
}

std::string threadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void yield() {
    if (const auto thisThread = ThreadHandle::getThis()) {
        thisThread->performInterruptionPointThrow();
    }
    std::this_thread::yield();
}

void sleep(const sol::stack_object& duration, const sol::stack_object& metric) {
    if (duration.valid()) {
        REQUIRE(duration.get_type() == sol::type::number)
                << "bad argument #1 to 'effil.sleep' (number expected, got "
                << luaTypename(duration) << ")";

        if (metric.valid())
        {
            REQUIRE(metric.get_type() == sol::type::string)
                    << "bad argument #2 to 'effil.sleep' (string expected, got "
                    << luaTypename(metric) << ")";
        }
        try {
            Notifier notifier;
            notifier.waitFor(fromLuaTime(duration.as<int>(),
                                         metric.as<sol::optional<std::string>>()));
        } RETHROW_WITH_PREFIX("effil.sleep");
    }
    else {
        yield();
    }
}

int pcall(lua_State* L)
{
    int status;
    luaL_checkany(L, 1);
    status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);

    const auto thisThread = ThreadHandle::getThis();
    if (thisThread && thisThread->command() == ThreadHandle::Command::Cancel) {
        lua_pushstring(L, ThreadCancelException::message);
        lua_error(L);
    }

    lua_pushboolean(L, (status == 0));
    lua_insert(L, 1);
    return lua_gettop(L);  /* return status + all results */
}

} // namespace this_thread
} // namespace effil
