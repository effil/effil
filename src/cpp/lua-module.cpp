#include "threading.h"
#include "shared-table.h"
#include "garbage-collector.h"

#include <lua.hpp>

using namespace effil;

namespace {

sol::object createThreadFactory(sol::this_state lua, const sol::function& func) {
    return sol::make_object(lua, std::make_unique<ThreadFactory>(func));
}

sol::object createShare(sol::this_state lua) { return sol::make_object(lua, getGC().create<SharedTable>()); }

} // namespace

extern "C" int luaopen_libeffil(lua_State* L) {
    sol::state_view lua(L);
    effil::LuaThread::getUserType(lua);
    SharedTable::getUserType(lua);
    ThreadFactory::getUserType(lua);
    sol::table public_api = lua.create_table_with("thread", createThreadFactory, //
                                                  "thread_id", threadId,         //
                                                  "sleep", sleep,                //
                                                  "yield", yield,                //
                                                  "share", createShare           //
                                                  );
    sol::stack::push(lua, public_api);
    return 1;
}
