#pragma once

#include "sol.hpp"
#include "lua-helpers.h"
#include "gc-data.h"
#include "function.h"

namespace effil {

namespace cache {

sol::table exportAPI(sol::state_view& lua);

void initStateCache(lua_State* lua);

void put(lua_State* lua, const Function& handle, const sol::function& func);
sol::object get(lua_State* lua, const Function& handle);

void put(lua_State* lua, const sol::function& luaFunc, const Function& func);
sol::optional<Function> get(lua_State* lua, const sol::function& func);

} // namespace cache
} // namespace effil
