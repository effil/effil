#pragma once

#include "lru_cache.h"
#include "function.h"
#include "lua-helpers.h"
#include "gc-data.h"
#include "sol.hpp"

namespace effil {

struct FunctionHash {
    inline size_t operator()(const Function& func) const {
        return std::hash<GCHandle>{}(func.handle());
    }
};

struct SolFunctionHash {
    inline size_t operator()(const sol::function& func) const {
        sol::stack::push(func.lua_state(), func);
        auto ptr = lua_topointer(func.lua_state(), -1);
        sol::stack::pop_n(func.lua_state(), 1);

        return std::hash<decltype(ptr)>{}(ptr);
    }
};

class Cache {
public:
    static sol::object exportAPI(sol::state_view& lua);
    static void create(lua_State* lua);
    static void destroy(lua_State* lua);
    static std::shared_ptr<Cache> instance(lua_State* lua);

    // Lua API
    void clear();
    int  getSize();
    int  getCapacity();
    void setCapacity(size_t capacity);
    bool isEnabled();
    bool remove(const sol::function& luaFunc);

    // internal C++ API
    void put(const Function& handle, const sol::function& func);
    sol::object get(const Function& handle);
    sol::optional<Function> get(const sol::function& func);

private:
    Cache(lua_State* state);
    Cache(const Cache&) = delete;

    lua_State* const state_;
    LruCache<Function, sol::function, FunctionHash> handleCache_;
    LruCache<sol::function, Function, SolFunctionHash> functionCache_;
};

} // namespace effil
