#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"
#include "channel.h"

#include <lua.hpp>

using namespace effil;

namespace {

sol::object createThread(const sol::this_state& lua,
                         const std::string& path,
                         const std::string& cpath,
                         int step,
                         const sol::function& function,
                         const sol::variadic_args& args) {
    return sol::make_object(lua, GC::instance().create<Thread>(path, cpath, step, function, args));
}

sol::object createTable(sol::this_state lua, const sol::optional<sol::object>& tbl) {
    if (tbl)
    {
        REQUIRE(tbl->get_type() == sol::type::table) << "Unexpected type for effil.table, table expected got: "
                                                     << lua_typename(lua, (int)tbl->get_type());
        return createStoredObject(*tbl)->unpack(lua);
    }
    return sol::make_object(lua, GC::instance().create<SharedTable>());
}

sol::object createChannel(const sol::stack_object& capacity, sol::this_state lua) {
    return sol::make_object(lua, GC::instance().create<Channel>(capacity));
}

SharedTable globalTable = GC::instance().create<SharedTable>();

std::string getLuaTypename(const sol::stack_object& obj)
{
    return luaTypename<>(obj);
}

} // namespace

extern "C"
#ifdef _WIN32
 __declspec(dllexport)
#endif
int luaopen_libeffil(lua_State* L) {
    sol::state_view lua(L);
    Thread::exportAPI(lua);
    SharedTable::exportAPI(lua);
    Channel::exportAPI(lua);
    sol::table publicApi = lua.create_table_with(
            "thread", createThread,
            "thread_id", threadId,
            "sleep", sleep,
            "yield", yield,
            "table", createTable,
            "rawset", SharedTable::luaRawSet,
            "rawget", SharedTable::luaRawGet,
            "table_size", SharedTable::luaSize,
            "setmetatable", SharedTable::luaSetMetatable,
            "getmetatable", SharedTable::luaGetMetatable,
            "G", sol::make_object(lua, globalTable),
            "gc", GC::exportAPI(lua),
            "channel", createChannel,
            "type", getLuaTypename,
            "pairs", SharedTable::globalLuaPairs,
            "ipairs", SharedTable::globalLuaIPairs,
            "allow_table_upvalue", lua_allow_table_upvalue
    );
    sol::stack::push(lua, publicApi);
    return 1;
}
