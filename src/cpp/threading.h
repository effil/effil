#pragma once

#include <sol.hpp>
#include "lua-helpers.h"
#include "function.h"

namespace effil {

// Lua this thread API
std::string threadId();
void yield();
void sleep(const sol::stack_object& duration, const sol::stack_object& metric);

class ThreadHandle;

class Thread : public GCObject {
public:
    Thread(const std::string& path,
           const std::string& cpath,
           int step,
           const sol::function& function,
           const sol::variadic_args& args);

    static void exportAPI(sol::state_view& lua);

    std::pair<sol::object, sol::object> status(const sol::this_state& state);
    std::pair<sol::object, sol::object> wait(const sol::this_state& state,
                                             const sol::optional<int>& duration,
                                             const sol::optional<std::string>& period);
    StoredArray get(const sol::optional<int>& duration,
                   const sol::optional<std::string>& period);
    bool cancel(const sol::this_state& state,
                const sol::optional<int>& duration,
                const sol::optional<std::string>& period);
    bool pause(const sol::this_state&,
               const sol::optional<int>& duration,
               const sol::optional<std::string>& period);
    void resume();

private:
    static void runThread(Thread, FunctionObject, effil::StoredArray);

    std::shared_ptr<ThreadHandle> handle_;
};

} // effil
