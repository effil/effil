#include "threading.h"
#include "shared-table.h"

#include <lua.hpp>

namespace {

static sol::object createThread(sol::this_state lua, sol::function func, const sol::variadic_args &args) noexcept {
    return sol::make_object(lua, std::make_unique<effil::LuaThread>(func, args));
}

static sol::object createShare(sol::this_state lua) noexcept {
    return sol::make_object(lua, std::make_unique<effil::SharedTable>());
}

} // namespace

extern "C" int luaopen_libeffil(lua_State *L) {
    sol::state_view lua(L);
    effil::LuaThread::getUserType(lua);
    effil::SharedTable::getUserType(lua);
    sol::table public_api = lua.create_table_with(
            "thread", createThread,
            "share", createShare
    );
    sol::stack::push(lua, public_api);
    return 1;
}

