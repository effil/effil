#pragma once

#include "utils.h"
#include <sol.hpp>

namespace effil {

inline void openAllLibs(sol::state& lua) {
    lua.open_libraries(sol::lib::base,
                       sol::lib::string,
                       sol::lib::package,
                       sol::lib::io,
                       sol::lib::os,
                       sol::lib::count,
                       sol::lib::bit32,
                       sol::lib::coroutine,
                       sol::lib::debug,
                       sol::lib::ffi,
                       sol::lib::jit,
                       sol::lib::math,
                       sol::lib::utf8,
                       sol::lib::table);
}

inline std::string dumpFunction(const sol::function& f) {
    sol::state_view lua(f.lua_state());
    sol::function dumper = lua["string"]["dump"];
    REQUIRE(dumper.valid() && dumper.get_type() == sol::type::function)
                << "Invalid string.dump() in state";
    return dumper(f);
}

inline sol::function loadString(const sol::state_view& lua, const std::string& str) {
    sol::function loader = lua["loadstring"];
    REQUIRE(loader.valid() && loader.get_type() == sol::type::function)
                << "Invalid loadstring function";
    return loader(str);
}

} // namespace effil