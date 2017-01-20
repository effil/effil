#include "threading.h"
#include "shared-table.h"

#include <lua.hpp>

static sol::object create_thread(sol::this_state lua, sol::function func, const sol::variadic_args& args)
{
    return sol::make_object(lua, std::make_unique<threading::LuaThread>(func, args));
}

static sol::object create_share(sol::this_state lua)
{
    return sol::make_object(lua, std::make_unique<share_data::SharedTable>());
}

extern "C" int luaopen_libwoofer(lua_State *L)
{
    sol::state_view lua(L);
    threading::LuaThread::get_user_type(lua);
    share_data::SharedTable::get_user_type(lua);
    sol::table public_api = lua.create_table_with(
        "thread", create_thread,
        "share",  create_share
    );
    sol::stack::push(lua, public_api);
    return 1;
}

