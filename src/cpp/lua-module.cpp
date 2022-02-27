#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"
#include "channel.h"
#include "thread_runner.h"

#include <lua.hpp>

using namespace effil;

namespace {

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

sol::object createMutex(sol::this_state lua) {
    return sol::make_object(lua, std::make_shared<SpinMutex>());
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

sol::object luaDump(sol::this_state lua, const sol::stack_object& obj) {
    if (obj.is<SharedTable>()) {
        BaseHolder::DumpCache cache;
        return obj.as<SharedTable>().luaDump(lua, cache);
    }
    else if (obj.get_type() == sol::type::table) {
        return obj;
    }

    throw effil::Exception() << "bad argument #1 to 'effil.dump' (table expected, got "
                             << luaTypename(obj) << ")";
}

sol::table createThreadRunner(sol::this_state state, const sol::stack_object& obj) {
    REQUIRE(obj.valid() && obj.get_type() == sol::type::function)
            << "bad argument #1 to 'effil.thread' (function expected, got "
            << luaTypename(obj) << ")";

    auto lua = sol::state_view(state);
    return sol::make_object(lua, GC::instance().create<ThreadRunner>(
        lua["package"]["path"],
        lua["package"]["cpath"],
        200,
        obj.as<sol::function>()
    ));
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
    ThreadRunner::exportAPI(lua);
    SpinMutex::exportAPI(lua);

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
            return sol::make_object(obj.lua_state(), "1.0.0");
        return sol::nil;
    };

    sol::usertype<EffilApiMarker> type("new", sol::no_constructor,
            "mutex",        createMutex,
            "thread",       createThreadRunner,
            "thread_id",    threadId,
            "sleep",        sleep,
            "yield",        yield,
            "table",        createTable,
            "rawset",       SharedTable::luaRawSet,
            "rawget",       SharedTable::luaRawGet,
            "setmetatable", SharedTable::luaSetMetatable,
            "getmetatable", SharedTable::luaGetMetatable,
            "channel",      createChannel,
            "type",         getLuaTypename,
            "pairs",        SharedTable::globalLuaPairs,
            "ipairs",       SharedTable::globalLuaIPairs,
            "next",         SharedTable::globalLuaNext,
            "size",         luaSize,
            "dump",         luaDump,
            "hardware_threads", std::thread::hardware_concurrency,
            sol::meta_function::index, luaIndex
    );

    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
    sol::stack::push(lua, EffilApiMarker());
    return 1;
}
