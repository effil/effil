#pragma once

#include "shared-table.h"
#include <sol.hpp>

namespace effil {

inline void bootstrapState(sol::state& lua) {
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table);
    SharedTable::getUserType(lua);
}

} // namespace
