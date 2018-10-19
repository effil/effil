#pragma once

#include "stored-object.h"
#include "utils.h"
#include <sol.hpp>

namespace effil {

class SharedTable;
class Channel;
class Thread;

std::string dumpFunction(const sol::function& f);
sol::function loadString(const sol::state_view& lua, const std::string& str,
                         const sol::optional<std::string>& source = sol::nullopt);
std::chrono::milliseconds fromLuaTime(int duration, const sol::optional<std::string>& period);

template <typename SolObject>
std::string luaTypename(const SolObject& obj) {
    if (obj.get_type() == sol::type::userdata) {
        if (obj.template is<SharedTable>())
            return "effil.table";
        else if (obj.template is<Channel>())
            return "effil.channel";
        else if (obj.template is<Thread>())
            return "effil.thread";
        else
            return "userdata";
    }
    else {
        return lua_typename(obj.lua_state(), (int)obj.get_type());
    }
}

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
    struct pusher<effil::StoredArray> {
        int push(lua_State* state, const effil::StoredArray& args) {
            int p = 0;
            for (const auto& i : args) {
                p += stack::push(state, i->unpack(sol::this_state{state}));
            }
            return p;
        }
    };
} // stack
} // sol
