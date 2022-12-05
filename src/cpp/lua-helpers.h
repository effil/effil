#pragma once

#include "stored-object.h"
#include "utils.h"
#include <sol/sol.hpp>

namespace effil {

class SharedTable;
class Channel;
class Thread;
class ThreadRunner;

std::string dumpFunction(const sol::function& f);
sol::function loadString(const sol::state_view& lua, const std::string& str,
                         const sol::optional<std::string>& source = sol::nullopt);
std::chrono::milliseconds fromLuaTime(int duration, const sol::optional<std::string>& period);

std::string luaTypename(const sol::stack_object& obj);
std::string luaTypename(const sol::object& obj);

typedef std::vector<effil::StoredObject> StoredArray;

class Timer {
public:
    Timer(const std::chrono::milliseconds& timeout);
    bool isFinished();
    std::chrono::milliseconds left();

private:
    std::chrono::milliseconds timeout_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

} // namespace effil

namespace sol {
namespace stack {
    template<>
    struct unqualified_pusher<effil::StoredArray> {
        int push(lua_State* state, const effil::StoredArray& args) {
            int p = 0;
            for (const auto& i : args) {
                p += stack::push(state, i->unpack(sol::this_state{state}));
            }
            return p;
        }
    };
} // namespace stack
} // namespace sol
