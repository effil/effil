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
    return sol::make_object(lua, GC::instance().create<ThreadView>(path, cpath, step, function, args));
}

sol::object createTable(sol::this_state lua, const sol::optional<sol::object>& tbl) {
    if (tbl)
    {
        REQUIRE(tbl->get_type() == sol::type::table) << "Unexpected type for effil.table, table expected got: "
                                                     << lua_typename(lua, (int)tbl->get_type());
        return createStoredObject(*tbl)->unpack(lua);
    }
    return sol::make_object(lua, GC::instance().create<SharedTableView>());
}

sol::object createChannel(const sol::stack_object& capacity, sol::this_state lua) {
    return sol::make_object(lua, GC::instance().create<ChannelView>(capacity));
}

SharedTableView globalTable = GC::instance().create<SharedTableView>();

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
    ThreadView::exportAPI(lua);
    SharedTableView::exportAPI(lua);
    ChannelView::exportAPI(lua);
    sol::table publicApi = lua.create_table_with(
            "thread", createThread,
            "thread_id", threadId,
            "sleep", sleep,
            "yield", yield,
            "table", createTable,
            "rawset", SharedTableView::luaRawSet,
            "rawget", SharedTableView::luaRawGet,
            "table_size", SharedTableView::luaSize,
            "setmetatable", SharedTableView::luaSetMetatable,
            "getmetatable", SharedTableView::luaGetMetatable,
            "G", sol::make_object(lua, globalTable),
            "gc", GC::exportAPI(lua),
            "channel", createChannel,
            "type", getLuaTypename,
            "pairs", SharedTableView::globalLuaPairs,
            "ipairs", SharedTableView::globalLuaIPairs,
            "allow_table_upvalues", luaAllowTableUpvalues
    );
    sol::stack::push(lua, publicApi);
    return 1;
}
