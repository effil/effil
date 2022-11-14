#pragma once

#include "lua-helpers.h"
#include "function.h"
#include "thread-handle.h"

#include <sol.hpp>

namespace effil {

class Thread : public GCObject<ThreadHandle> {
public:
    static void exportAPI(sol::state_view& lua);

    StoredArray status(const sol::this_state& state);
    StoredArray wait(const sol::this_state& state,
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
    Thread() = default;
    void initialize(
        const std::string& path,
        const std::string& cpath,
        int step,
        const sol::function& function,
        const sol::variadic_args& args);
    friend class GC;

private:
    static void runThread(Thread, Function, effil::StoredArray);
};

} // effil
