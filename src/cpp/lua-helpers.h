#pragma once

#include "stored-object.h"
#include "utils.h"
#include <sol.hpp>

namespace effil {

// TODO: make function more reliable
// string.dump can be changed by user
// TODO: Add cache for each state
inline std::string dumpFunction(const sol::function& f) {
    sol::state_view lua(f.lua_state());
    sol::function dumper = lua["string"]["dump"];
    REQUIRE(dumper.valid() && dumper.get_type() == sol::type::function)
                << "Invalid string.dump() in state";
    return dumper(f);
}

// TODO: make function more reliable
// loadstring can be changed by user
// TODO: Add cache for each state
inline sol::function loadString(const sol::state_view& lua, const std::string& str) {
    sol::function loader = lua["loadstring"];
    REQUIRE(loader.valid() && loader.get_type() == sol::type::function)
                << "Invalid loadstring function";
    return loader(str);
}

typedef std::vector<effil::StoredObject> MultipleReturn;

} // namespace effil

namespace sol {
namespace stack {
    template<>
    struct pusher<effil::MultipleReturn> {
        int push(lua_State* state, const effil::MultipleReturn& args) {
            int p = 0;
            for (const auto& i : args) {
                p += stack::push(state, i->unpack(sol::this_state{state}));
            }
            return p;
        }
    };
} // stack
} // sol
