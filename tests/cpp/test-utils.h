#pragma once

#include "shared-table.h"
#include "lua-helpers.h"
#include <sol.hpp>

namespace effil {

inline void bootstrapState(sol::state& lua) {
    luaL_openlibs(lua);
    SharedTable::getUserType(lua);
}

} // namespace effil
