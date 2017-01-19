extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "threading.h"
#include "shared-table.h"

extern "C" int luaopen_libwoofer(lua_State *L)
{
    sol::state_view lua(L);
    auto thread_obj = threading::LuaThread::get_user_type(lua);
    auto share_obj = share_data::SharedTable::get_user_type(lua);
    sol::table public_api = lua.create_table_with(
        "thread", thread_obj,
        "share",  share_obj
    );
    sol::stack::push(lua, public_api);
    return 1;
}

