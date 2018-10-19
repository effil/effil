#include "lua-helpers.h"

namespace effil {

namespace {

std::string luaError(int errCode) {
    switch(errCode) {
        case LUA_ERRSYNTAX: return "Invalid syntax (LUA_ERRSYNTAX)";
        case LUA_ERRMEM:    return "Memory allocation error (LUA_ERRMEM)";
        case LUA_ERRRUN:    return "Execution error (LUA_ERRRUN)";
        case LUA_ERRGCMM:   return "Error in __gc method (LUA_ERRGCMM)";
        case LUA_ERRERR:    return "Recursive error (LUA_ERRERR)";
        default: return "Unknown (" + std::to_string(errCode) + ")";
    }
}

int dumpMemoryWriter(lua_State*, const void* batch, size_t batchSize, void* storage) {
    if (storage == nullptr)
        return 1;
    if (batchSize && batch) {
        std::string& buff = *reinterpret_cast<std::string*>(storage);
        const char* newData = reinterpret_cast<const char*>(batch);
        buff.insert(buff.end(), newData, newData + batchSize);
    }
    return 0;
}

} // namespace

std::string dumpFunction(const sol::function& f) {
    sol::state_view lua(f.lua_state());
    sol::stack::push(lua, f);
    std::string result;
#if LUA_VERSION_NUM == 503
    int ret = lua_dump(lua, dumpMemoryWriter, &result, 0 /* not strip debug info*/);
#else
    int ret = lua_dump(lua, dumpMemoryWriter, &result);
#endif
    REQUIRE(ret == LUA_OK) << "Unable to dump Lua function: " << luaError(ret);
    sol::stack::remove(lua, -1, 1);
    return result;
}

sol::function loadString(const sol::state_view& lua, const std::string& str,
                         const sol::optional<std::string>& source /* = sol::nullopt*/) {
    int ret = luaL_loadbuffer(lua, str.c_str(), str.size(), source ? source.value().c_str() : nullptr);
    REQUIRE(ret == LUA_OK) << "Unable to load function from string: " << luaError(ret);
    return sol::stack::pop<sol::function>(lua);
}

std::chrono::milliseconds fromLuaTime(int duration, const sol::optional<std::string>& period) {
    using namespace std::chrono;

    REQUIRE(duration >= 0) << "invalid duration interval: " << duration;

    std::string metric = period ? period.value() : "s";
    if (metric == "ms") return milliseconds(duration);
    else if (metric == "s") return seconds(duration);
    else if (metric == "m") return minutes(duration);
    else throw sol::error("invalid time metric: " + metric);
}

using namespace std::chrono;

Timer::Timer(const milliseconds& timeout)
    : timeout_(timeout), startTime_(high_resolution_clock::now())
{}

bool Timer::isFinished() {
    return left() == milliseconds(0);
}

milliseconds Timer::left() {
    const auto diff = high_resolution_clock::now() - startTime_;
    return timeout_ > diff ? duration_cast<milliseconds>((timeout_ - diff)):
                             milliseconds(0);
}


} // namespace effil
