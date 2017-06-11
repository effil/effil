#pragma once

#include "stored-object.h"
#include "utils.h"
#include <sol.hpp>

namespace effil {

std::string dumpFunction(const sol::function& f);
sol::function loadString(const sol::state_view& lua, const std::string& str);
std::chrono::milliseconds fromLuaTime(int duration, const sol::optional<std::string>& period);

typedef std::vector<effil::StoredObject> StoredArray;

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
