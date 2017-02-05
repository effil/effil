#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"

#include <lua.hpp>

using namespace effil;

namespace {

static sol::object createThread(sol::this_state lua, sol::function func, const sol::variadic_args &args) noexcept {
    return sol::make_object(lua, std::make_unique<LuaThread>(func, args));
}

static sol::object createShare(sol::this_state lua) noexcept {
    return sol::make_object(lua, getGC().create<SharedTable>());
}

} // namespace

extern "C" int luaopen_libeffil(lua_State *L) {
    sol::state_view lua(L);
    LuaThread::getUserType(lua);
    SharedTable::getUserType(lua);
    sol::table public_api = lua.create_table_with(
            "thread", createThread,
            "share", createShare
    );
    sol::stack::push(lua, public_api);
    return 1;
}

