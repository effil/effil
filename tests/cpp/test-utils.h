#pragma once

#include "shared-table.h"
#include "utils.h"
#include <sol.hpp>

namespace effil {

inline void bootstrapState(sol::state& lua) {
    openAllLibs(lua);
    SharedTable::getUserType(lua);
}

} // namespace
