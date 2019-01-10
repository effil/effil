#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"
#include "channel.h"

#include <lua.hpp>

using namespace effil;

namespace {

sol::object createTable(sol::this_state lua, const sol::optional<sol::object>& tbl) {
    if (tbl)
    {
        REQUIRE(tbl->get_type() == sol::type::table) << "Unexpected type for effil.table, table expected got: "
                                                     << lua_typename(lua, (int)tbl->get_type());
        return createStoredObject(*tbl).unpack(lua);
    }
    return sol::make_object(lua, GC::instance().create<SharedTable>());
}

sol::object createChannel(const sol::stack_object& capacity, sol::this_state lua) {
    return sol::make_object(lua, GC::instance().create<Channel>(capacity));
}

SharedTable globalTable = GC::instance().create<SharedTable>();

std::string getLuaTypename(const sol::stack_object& obj) {
    return luaTypename<>(obj);
}

size_t luaSize(const sol::stack_object& obj) {
    if (obj.is<SharedTable>())
        return SharedTable::luaSize(obj);
    else if (obj.is<Channel>())
        return obj.as<Channel>().size();

    throw effil::Exception() << "Unsupported type "
                             << luaTypename(obj) << " for effil.size()";
}

sol::table luaThreadConfig(sol::this_state state, const sol::stack_object& obj) {
    REQUIRE(obj.valid() && obj.get_type() == sol::type::function)
            << "bad argument #1 to 'effil.thread' (function expected, got "
            << luaTypename(obj) << ")";

    auto lua = sol::state_view(state);
    const sol::function func = obj.as<sol::function>();

    auto config = lua.create_table_with(
        "path",  lua["package"]["path"],
        "cpath", lua["package"]["cpath"],
        "step",  200
    );

    auto meta = lua.create_table_with();
    meta[sol::meta_function::call] = [func](sol::this_state lua,
                const sol::stack_table& self, const sol::variadic_args& args)
    {
        return sol::make_object(lua, GC::instance().create<Thread>(
                self["path"], self["cpath"], self["step"], func, args));
    };

    config[sol::metatable_key] = meta;
    return config;
}

} // namespace

extern "C"
#ifdef _WIN32
 __declspec(dllexport)
#endif
int luaopen_effil(lua_State* L) {
    sol::state_view lua(L);
    Thread::exportAPI(lua);
    SharedTable::exportAPI(lua);
    Channel::exportAPI(lua);

    const sol::table  gcApi     = GC::exportAPI(lua);
    const sol::object gLuaTable = sol::make_object(lua, globalTable);

    const auto luaIndex = [gcApi, gLuaTable](
            const sol::stack_object& obj, const std::string& key) -> sol::object
    {
        if (key == "G")
            return gLuaTable;
        else if (key == "gc")
            return gcApi;
        else if (key == "version")
            return sol::make_object(obj.lua_state(), "0.1.0");
        return sol::nil;
    };

    sol::usertype<EffilApiMarker> type("new", sol::no_constructor,
            "thread",       luaThreadConfig,
            "thread_id",    threadId,
            "sleep",        sleep,
            "yield",        yield,
            "table",        createTable,
            "rawset",       SharedTable::luaRawSet,
            "rawget",       SharedTable::luaRawGet,
            "setmetatable", SharedTable::luaSetMetatable,
            "getmetatable", SharedTable::luaGetMetatable,
            "reserve",      SharedTable::luaReserve,
            "channel",      createChannel,
            "type",         getLuaTypename,
            "pairs",        SharedTable::globalLuaPairs,
            "ipairs",       SharedTable::globalLuaIPairs,
            "size",         luaSize,
            "hardware_threads",        std::thread::hardware_concurrency,
            sol::meta_function::index, luaIndex
    );

    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
    sol::stack::push(lua, EffilApiMarker());
    return 1;
}
