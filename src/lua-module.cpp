extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "threading.h"
#include "shared-table.h"

sol::table core::init_state(sol::state_view& lua)
{
    sol::usertype<LuaThread> thread_api(
        sol::call_construction(), sol::constructors<sol::types<sol::function, sol::variadic_args>>(),
        "join",      &LuaThread::join,
        "thread_id", &LuaThread::thread_id
    );
    sol::stack::push(lua, thread_api);
    auto thread_obj = sol::stack::pop<sol::object>(lua);

    sol::usertype<core::SharedTable> share_api(
        sol::call_construction(), sol::default_constructor,
        sol::meta_function::new_index, &core::SharedTable::luaSet,
        sol::meta_function::index,     &core::SharedTable::luaGet
    );
    sol::stack::push(lua, share_api);
    auto share_obj = sol::stack::pop<sol::object>(lua);

    sol::table public_api = lua.create_table_with(
        "thread", thread_obj,
        "share",  share_obj
    );
    return public_api;
}

extern "C" int luaopen_libwoofer(lua_State *L)
{
    sol::state_view lua(L);
    sol::stack::push(lua, core::init_state(lua));
    return 1;
}

