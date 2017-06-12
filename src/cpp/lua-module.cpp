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
    return sol::make_object(lua, std::make_shared<Thread>(path, cpath, step, function, args));
}

sol::object createTable(sol::this_state lua) {
    return sol::make_object(lua, GC::instance().create<SharedTable>());
}

sol::object createChannel(sol::optional<int> capacity, sol::this_state lua) {
    return sol::make_object(lua, GC::instance().create<Channel>(capacity));
}

SharedTable globalTable = GC::instance().create<SharedTable>();

std::string userdataType(const sol::object& something) {
    assert(something.get_type() == sol::type::userdata);
    if (something.template is<SharedTable>()) {
        return "effil.table";
    } else if (something.template is<Channel>()) {
        return "effil.channel";
    } else if (something.template is<std::shared_ptr<Thread>>()) {
        return "effil.thread";
    } else {
        return "userdata";
    }
}

} // namespace

extern "C" int luaopen_libeffil(lua_State* L) {
    sol::state_view lua(L);
    Thread::getUserType(lua);
    SharedTable::getUserType(lua);
    Channel::getUserType(lua);
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
            "G", sol::make_object(lua, globalTable),
            "getmetatable", SharedTable::luaGetMetatable,
            "gc", GC::getLuaApi(lua),
            "channel", createChannel,
            "userdata_type", userdataType
    );
    sol::stack::push(lua, publicApi);
    return 1;
}
