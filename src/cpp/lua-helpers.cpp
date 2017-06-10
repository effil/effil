#include "lua-helpers.h"

namespace effil {

std::string luaError(int errCode)
{
#define DECL_ERROR(id, mess) \
    case id: return std::string(mess) + "(" #id ")"

    switch(errCode)
    {
        DECL_ERROR(LUA_ERRSYNTAX, "Invalid syntax");
        DECL_ERROR(LUA_ERRMEM,    "Memory allocation error");
        DECL_ERROR(LUA_ERRRUN,    "Execution error");
        DECL_ERROR(LUA_ERRGCMM,   "Error in __gc method");
        DECL_ERROR(LUA_ERRERR,    "Recursive error");
        default: return "Unknown";
    }
#undef DECL_ERROR
}

static int dumpMemoryWriter(lua_State*, const void* p, size_t sz, void* ud) {
    if (ud == nullptr || p == nullptr)
        return 1;
    if (sz) {
        std::string& buff = *reinterpret_cast<std::string*>(ud);
        const char* newData = reinterpret_cast<const char*>(p);
        buff.insert(buff.end(), newData, newData + sz);
    }
    return 0;
}

std::string dumpFunction(const sol::function& f) {
    sol::state_view lua(f.lua_state());
    sol::stack::push(lua, f);
    std::string result;
    int ret = lua_dump(lua, dumpMemoryWriter, &result);
    REQUIRE(ret == LUA_OK) << "Unable to dump Lua function: " << luaError(ret);
    sol::stack::remove(lua, -1, 1);
    return result;
}

sol::function loadString(const sol::state_view& lua, const std::string& str) {
    int ret = luaL_loadbuffer(lua, str.c_str(), str.size(), nullptr);
    REQUIRE(ret == LUA_OK) << "Unable to load function from string: " << luaError(ret);
    return sol::stack::pop<sol::function>(lua);
}

std::chrono::milliseconds fromLuaTime(int duration, const sol::optional<std::string>& period) {
    using namespace std::chrono;

    REQUIRE(duration >= 0) << "Invalid duration interval: " << duration;

    std::string metric = period ? period.value() : "s";
    if (metric == "ms") return milliseconds(duration);
    else if (metric == "s") return seconds(duration);
    else if (metric == "m") return minutes(duration);
    else throw sol::error("invalid time identification: " + metric);
}

} // namespace effil
