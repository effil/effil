#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"

#include <lua.hpp>

using namespace effil;

namespace {

sol::object createThread(const sol::this_state& lua,
                         const std::string& path,
                         const std::string& cpath,
                         bool stepwise,
                         unsigned int step,
                         const sol::function& function,
                         const sol::variadic_args& args) {
    return sol::make_object(lua, std::make_shared<Thread>(path, cpath, stepwise, step, function, args));
}

sol::object createTable(sol::this_state lua) {
    return sol::make_object(lua, GC::instance().create<SharedTable>());
}

} // namespace

extern "C" int luaopen_libeffil(lua_State* L) {
    sol::state_view lua(L);
    Thread::getUserType(lua);
    SharedTable::getUserType(lua);
    sol::table publicApi = lua.create_table_with(
            "thread", createThread,
            "thread_id", threadId,
            "sleep", sleep,
            "yield", yield,
            "table", createTable,
            "rawset", SharedTable::luaRawSet,
            "rawget", SharedTable::luaRawGet,
            "size", SharedTable::luaSize,
            "setmetatable", SharedTable::luaSetMetatable,
            "getmetatable", SharedTable::luaGetMetatable,
            "gc", GC::getLuaApi(lua)
    );
    sol::stack::push(lua, publicApi);
    return 1;
}
