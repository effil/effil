#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"
#include "channel.h"
#include "thread_runner.h"

#include <lua.hpp>

using namespace effil;

namespace {


SharedTable globalTable = GC::instance().create<SharedTable>();

std::string getLuaTypename(const sol::stack_object& obj) {
    return luaTypename(obj);
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

} // namespace

extern "C"
#ifdef _WIN32
 __declspec(dllexport)
#endif
int luaopen_effil(lua_State* L) {
    sol::state_view lua(L);

    static char uniqueRegistryKeyValue = 'a';
    char* uniqueRegistryKey = &uniqueRegistryKeyValue;
    const auto registryKey = sol::make_light(uniqueRegistryKey);

    if (lua.registry()[registryKey] == 1) {
        sol::stack::push(lua, EffilApiMarker());
        return 1;
    }

    Thread::exportAPI(lua);

    // const auto topBefore = lua_gettop(L);

    auto type = lua.new_usertype<EffilApiMarker>(sol::no_constructor);

    type["version"] = sol::make_object(lua, "1.0.0");
    type["G"] = sol::make_object(lua, globalTable);

    type["channel"] = Channel::exportAPI(lua);
    type["table"] = SharedTable::exportAPI(lua);
    type["thread"] = ThreadRunner::exportAPI(lua);
    type["gc"] = GC::exportAPI(lua);

    type["thread_id"] = this_thread::threadId,
    type["sleep"] = this_thread::sleep;
    type["yield"] = this_thread::yield;
    type["rawset"] = SharedTable::luaRawSet;
    type["rawget"] = SharedTable::luaRawGet;
    type["setmetatable"] = SharedTable::luaSetMetatable;
    type["getmetatable"] = SharedTable::luaGetMetatable;
    type["type"] = getLuaTypename;
    type["pairs"] = SharedTable::globalLuaPairs;
    type["ipairs"] = SharedTable::globalLuaIPairs;
    type["next"] = SharedTable::globalLuaNext;
    type["size"] = luaSize;
    type["dump"] = luaDump;
    type["hardware_threads"] = std::thread::hardware_concurrency;

    // if (topBefore != lua_gettop(L))
    //     sol::stack::pop_n(L, lua_gettop(L) - topBefore);

    lua.registry()[registryKey] = 1;

    sol::stack::push(lua, EffilApiMarker());
    return 1;
}
