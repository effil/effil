#include "cache.h"

namespace effil {

typedef int LuaRef;

static const int CACHE_REGISTRY_INDEX_ADDR = 0;
static const LuaRef CACHE_REGISTRY_INDEX =
        reinterpret_cast<intptr_t>(&CACHE_REGISTRY_INDEX_ADDR);

constexpr size_t DEFAULT_CACHE_SIZE = 1024;

Cache::Cache(lua_State* state)
    : state_(state), handleCache_(DEFAULT_CACHE_SIZE),
      functionCache_(DEFAULT_CACHE_SIZE)
{}

void Cache::create(lua_State* lua) {
    if (instance(lua) != nullptr)
    {
        DEBUG("cache") << "already initialized: " << std::hex << lua;
        return;
    }

    sol::table reg(lua, LUA_REGISTRYINDEX);
    reg[CACHE_REGISTRY_INDEX] = std::shared_ptr<Cache>(new Cache(lua));
    DEBUG("cache") << "Cache for state " << std::hex << lua << " was initialized";
}

void Cache::destroy(lua_State* lua) {
    DEBUG("cache") << "remove from Lua state " << std::hex << lua;
    sol::table reg(lua, LUA_REGISTRYINDEX);
    reg[CACHE_REGISTRY_INDEX] = sol::nil;
}

std::shared_ptr<Cache> Cache::instance(lua_State* lua) {
    sol::table reg(lua, LUA_REGISTRYINDEX);
    sol::object obj = reg[CACHE_REGISTRY_INDEX];
    return obj.valid() ? obj.as<std::shared_ptr<Cache>>() : nullptr;
}

void Cache::clear() {
    DEBUG("cache") << "clear cache for Lua state " << std::hex << state_;
    handleCache_.clear();
    functionCache_.clear();
}

int Cache::getSize() {
    return handleCache_.size() + functionCache_.size();
}

int Cache::getCapacity() {
    return handleCache_.capacity();
}

void Cache::setCapacity(size_t capacity) {
    DEBUG("cache") << "set cache capacity for Lua state " << std::hex << state_;
    handleCache_.setCapacity(capacity);
    functionCache_.setCapacity(capacity);
}

bool Cache::remove(const sol::function& luaFunc) {
    DEBUG("cache") << "remove function " << luaFunc.registry_index()
                   << " from Lua state " << std::hex << state_;

    if (functionCache_.remove(luaFunc)) {
        DEBUG("cache") << "function " << luaFunc.registry_index()
                       << " was removed from Lua state " << std::hex << state_;
        return true;
    }
    else {
        DEBUG("cache") << "function " << luaFunc.registry_index()
                       << "not found in Lua state" << std::hex << state_;
        return false;
    }
}

void Cache::put(const Function& func, const sol::function& luaFunc) {
    DEBUG("cache") << "put handle " << std::hex << func.handle()
                   << " in state " << std::hex << state_
                   << " with function " << luaFunc.registry_index();

    handleCache_.put(func, luaFunc);
    functionCache_.put(luaFunc, func);
}

sol::object Cache::get(const Function& func) {
    DEBUG("cache") << "get handle " << std::hex << func.handle()
                   << " from " << std::hex << state_;

    const auto optFunc = handleCache_.get(func);
    if (!optFunc) {
        DEBUG("cache") << "handle not found " << std::hex << func.handle()
                       << " in " << std::hex << state_;
        return sol::nil;
    }

    DEBUG("cache") << "handle found " << std::hex << func.handle()
                   << " in " << std::hex << state_
                   << " under address " << optFunc->registry_index();
    return *optFunc;
}

sol::optional<Function> Cache::get(const sol::function& func) {
    DEBUG("cache") << "get function " << func.registry_index()
                   << " from " << std::hex << state_;

    const auto optFunc = functionCache_.get(func);
    if (!optFunc) {
        DEBUG("cache") << "function not found " << func.registry_index()
                       << " in " << std::hex << state_;
        return sol::nullopt;
    }

    DEBUG("cache") << "function found " << func.registry_index()
                   << " in " << std::hex << state_
                   << " with handle " << std::hex << optFunc->handle();

    return optFunc;
}

sol::object Cache::exportAPI(sol::state_view& lua) {
    sol::table api = lua.create_table_with();

    api["enable"]  = [](sol::this_state s) { Cache::create(s); };
    api["enabled"] = [](sol::this_state s) { return instance(s) != nullptr; };
    api["disable"] = [](sol::this_state s) { Cache::destroy(s); };
    api["clear"]   = [](sol::this_state s) {
            if (auto cache = instance(s))
                cache->clear();
    };
    api["size"] = [](sol::this_state s) -> sol::optional<int> {
        if (auto cache = instance(s))
            return cache->getSize();
        return sol::nullopt;
    };
    api["capacity"] = [](sol::this_state s, const sol::stack_object& value)
            -> sol::optional<int> {
        auto cache = instance(s);
        if (cache == nullptr)
            return sol::nullopt;

        auto previous = cache->getCapacity();
        if (value.valid()) {
            REQUIRE(value.get_type() == sol::type::number)
                    << "bad argument #1 to 'effil.cache.capacity' "
                       "(number expected, got " << luaTypename(value) << ")";
            REQUIRE(value.as<int>() >= 0)
                    << "effil.cache.capacity: invalid capacity value = "
                    << value.as<int>();
            cache->setCapacity(value.as<size_t>());
        }
        return previous;
    };
    api["remove"] = [](sol::this_state s, const sol::stack_object& value)
            -> sol::optional<bool> {
        REQUIRE(value.get_type() == sol::type::function)
                << "bad argument #1 to 'effil.cache.remove' "
                   "(function expected, got " << luaTypename(value) << ")";

        if (auto cache = instance(s))
            return cache->remove(value.as<sol::function>());
        return sol::nullopt;
    };

    return api;
}

} // namespace effil

