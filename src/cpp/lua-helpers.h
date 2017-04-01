#pragma once

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

} // namespace effil