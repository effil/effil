#include "threading.h"
#include "shared-table.h"

#include <lua.hpp>

namespace {

sol::object createThreadFactory(sol::this_state lua, const sol::function& func) {
    return sol::make_object(lua, std::make_unique<effil::ThreadFactory>(func));
}

sol::object createShare(sol::this_state lua) {
    return sol::make_object(lua, std::make_unique<effil::SharedTable>());
}

} // namespace

extern "C" int luaopen_libeffil(lua_State *L) {
    sol::state_view lua(L);
    effil::LuaThread::getUserType(lua);
    effil::SharedTable::getUserType(lua);
    effil::ThreadFactory::getUserType(lua);
    sol::table public_api = lua.create_table_with(
            "thread", createThreadFactory,
            "thread_id", effil::threadId,
            "sleep", effil::sleep,
            "yield", effil::yield,
            "share", createShare
    );
    sol::stack::push(lua, public_api);
    return 1;
}

