#include "cache.h"

#include <list>
#include <unordered_map>

namespace effil {
namespace cache {

namespace {

typedef int LuaRef;
static thread_local LuaRef thisThreadCacheRef = LUA_NOREF;
const size_t defaultCacheSize = 1000;

template<typename Key, typename Value, typename Destructor, typename Hash = std::hash<Key>>
class LruCache {
public:
    LruCache(Destructor destructor, size_t size)
        : capacity_(size), destructor_(destructor)
    {}

    void put(const Key& key, const Value& value) {
        if (itemsMap_.find(key) != itemsMap_.end())
            return;

        auto stat = itemsMap_.emplace(key, itemsList_.end());
        if (stat.second) { // was inserted
            itemsList_.emplace_front(std::make_pair(key, value));
            stat.first->second = itemsList_.begin();
        }

        if (capacity_ > 0 && itemsList_.size() > capacity_)
            removeLast();
    }

    sol::optional<Value> get(const Key& key) {
        const auto mapIter = itemsMap_.find(key);
        if (mapIter != itemsMap_.end()) {
            itemsList_.splice(itemsList_.begin(), itemsList_, mapIter->second);
            return mapIter->second->second;
        }
        return sol::nullopt;
    }

    void clear() {
        for (const auto& pair: itemsList_)
            destructor_(pair.second);

        itemsMap_.clear();
        itemsList_.clear();
    }

    bool remove(const Key& key) {
        const auto iter = itemsMap_.find(key);
        if (iter == itemsMap_.end())
                return false;

        destructor_(iter->second->second);
        itemsList_.erase(iter->second);
        itemsMap_.erase(iter);
        return true;
    }

    size_t size() {
        return itemsList_.size();
    }

    void setCapacity(size_t capacity) {
        capacity_ = capacity;

        while (capacity > 0 && itemsList_.size() > capacity)
            removeLast();
    }

    size_t capacity() {
        return capacity_;
    }

private:
    void removeLast() {
        const auto& pair = itemsList_.back();
        destructor_(pair.second);
        itemsMap_.erase(pair.first);
        itemsList_.pop_back();
    }

    typedef std::list<std::pair<Key, Value>> ItemsList;

    size_t capacity_;
    Destructor destructor_;
    ItemsList itemsList_;
    std::unordered_map<Key, typename ItemsList::iterator, Hash> itemsMap_;
};

struct HandleCacheContext {
    Function object;
    LuaRef   ref;
};

class LuaRegDestructor {
public:
    LuaRegDestructor(lua_State* lua) : lua_(lua) {}

    void operator() (LuaRef ref) {
        luaL_unref(lua_, LUA_REGISTRYINDEX, ref);
    }

    void operator() (const HandleCacheContext& ctx) {
        luaL_unref(lua_, LUA_REGISTRYINDEX, ctx.ref);
    }

private:
    lua_State* lua_;
};

struct FunctionHash {
    size_t operator()(const Function& func) const {
        return std::hash<GCHandle>{}(func.handle());
    }
};

struct StateCachePair {
    LruCache<Function, LuaRef, LuaRegDestructor, FunctionHash> handleCache;
    LruCache<const void*, HandleCacheContext, LuaRegDestructor> functionCache;

    StateCachePair(size_t capacity, const LuaRegDestructor& destructor)
        : handleCache(destructor, capacity), functionCache(destructor, capacity)
    {}
};

StateCachePair& getStateCache(lua_State* lua) {
    assert(thisThreadCacheRef != LUA_NOREF);

    sol::object obj(lua, sol::ref_index(thisThreadCacheRef));
    return obj.as<StateCachePair>();
}

void removeStateCache(lua_State* lua) {
    DEBUG("cache") << "remove Lua state " << std::hex << lua;
    if (thisThreadCacheRef != LUA_NOREF) {
        getStateCache(lua).handleCache.clear();
        getStateCache(lua).functionCache.clear();
        luaL_unref(lua, LUA_REGISTRYINDEX, thisThreadCacheRef);
        thisThreadCacheRef = LUA_NOREF;
    }
}

void clearStateCache(lua_State* lua) {
    DEBUG("cache") << "clear cache for Lua state " << std::hex << lua;
    if (thisThreadCacheRef != LUA_NOREF) {
        getStateCache(lua).handleCache.clear();
        getStateCache(lua).functionCache.clear();
    }
}

int getCacheSize(lua_State* lua) {
    if (thisThreadCacheRef != LUA_NOREF) {
        return getStateCache(lua).handleCache.size() +
               getStateCache(lua).functionCache.size();
    }
    return 0;
}

int getCacheCapacity(lua_State* lua) {
    if (thisThreadCacheRef != LUA_NOREF)
        return getStateCache(lua).handleCache.capacity();
    return 0;
}

void setCacheCapacity(lua_State* lua, size_t capacity) {
    DEBUG("cache") << "set cache capacity for Lua state " << std::hex << lua;
    if (thisThreadCacheRef != LUA_NOREF) {
        getStateCache(lua).handleCache.setCapacity(capacity);
        getStateCache(lua).functionCache.setCapacity(capacity);
    }
}

bool isStateCacheEnabled(lua_State* lua) {
    bool enabled = thisThreadCacheRef != LUA_NOREF;
    DEBUG("cache") << "is " << (enabled ? "enabled" : "disabled")
                   << " for Lua state " << std::hex << lua;
    return enabled;
}

bool removeFromCache(lua_State* lua, const sol::function& luaFunc) {
    luaFunc.push();
    const void* ptr = lua_topointer(lua, -1);
    luaFunc.pop();

    DEBUG("cache") << "remove function " << std::hex << ptr
                   << " from Lua state " << std::hex << lua;

    if (thisThreadCacheRef == LUA_NOREF) {
        DEBUG("cache") << "not enabled for state " << std::hex << lua;
        return false;
    }

    if (getStateCache(lua).functionCache.remove(ptr)) {
        DEBUG("cache") << "function " << std::hex << ptr
                       << " was removed from Lua state " << std::hex << lua;
        return true;
    }
    else {
        DEBUG("cache") << "function " << std::hex << ptr
                       << "not found in Lua state" << std::hex << lua;
        return false;
    }
}

} // namespace

void initStateCache(lua_State* lua) {
    DEBUG("cache") << "init Lua state " << std::hex << lua;
    if (thisThreadCacheRef == LUA_NOREF) {
        LuaRegDestructor destructor(lua);
        auto cache = std::make_shared<StateCachePair>(defaultCacheSize, destructor);
        sol::stack::push(lua, cache);
        thisThreadCacheRef = luaL_ref(lua, LUA_REGISTRYINDEX);
    }
    DEBUG("cache") << "Lua state " << std::hex << lua << " was initialized";
}

void put(lua_State* lua, const Function& func, const sol::function& luaFunc) {
    DEBUG("cache") << "put handle " << std::hex << func.handle()
                   << " in state " << std::hex << lua
                   << " with function " << std::hex << luaFunc.registry_index();

    if (thisThreadCacheRef == LUA_NOREF) {
        DEBUG("cache") << "not enabled for state " << std::hex << lua;
        return;
    }

    luaFunc.push();
    const auto ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    getStateCache(lua).handleCache.put(func, ref);

    DEBUG("cache") << "handle pushed " << std::hex << func.handle()
                   << " in state " << std::hex << lua
                   << " under address " << std::hex << ref;
}

sol::object get(lua_State* lua, const Function& func) {
    DEBUG("cache") << "get handle " << std::hex << func.handle()
                   << " from " << std::hex << lua;

    if (thisThreadCacheRef == LUA_NOREF) {
        DEBUG("cache") << "not enabled for state " << std::hex << lua;
        return sol::nil;
    }

    const auto ref = getStateCache(lua).handleCache.get(func);
    if (!ref) {
        DEBUG("cache") << "handle not found " << std::hex << func.handle()
                       << " in " << std::hex << lua;
        return sol::nil;
    }

    DEBUG("cache") << "handle found " << std::hex << func.handle()
                   << " in " << std::hex << lua
                   << " under address " << std::hex << ref.value();

    return sol::object(lua, sol::ref_index(ref.value()));
}

void put(lua_State* lua, const sol::function& luaFunc, const Function& func)
{
    luaFunc.push();
    const void* ptr = lua_topointer(lua, -1);

    DEBUG("cache") << "put function " << std::hex << ptr
                   << " in state " << std::hex << lua
                   << " with handle " << std::hex << func.handle();

    if (thisThreadCacheRef == LUA_NOREF) {
        DEBUG("cache") << "not enabled for state " << std::hex << lua;
        return;
    }

    const auto ref = luaL_ref(lua, LUA_REGISTRYINDEX);

    getStateCache(lua).functionCache.put(ptr, { func, ref });

    DEBUG("cache") << "function pushed " << std::hex << ptr
                   << " in state " << std::hex << lua
                   << " under adress " << std::hex << ref;
}

sol::optional<Function> get(lua_State* lua, const sol::function& func)
{
    func.push();
    const void* ptr = lua_topointer(lua, -1);
    func.pop();

    DEBUG("cache") << "get function " << std::hex << ptr
                   << " from " << std::hex << lua;

    if (thisThreadCacheRef == LUA_NOREF) {
        DEBUG("cache") << "not enabled for state " << std::hex << lua;
        return sol::nullopt;
    }

    auto ctx = getStateCache(lua).functionCache.get(ptr);
    if (!ctx) {
        DEBUG("cache") << "function not found " << std::hex << ptr
                       << " in " << std::hex << lua;
        return sol::nullopt;
    }

    DEBUG("cache") << "function found " << std::hex << ptr
                   << " in " << std::hex << lua
                   << " with handle " << std::hex << ctx->object.handle();

    return ctx->object;
}

sol::table exportAPI(sol::state_view& lua) {
    sol::table api = lua.create_table_with();

    api["clear"]    = [](sol::this_state s){ clearStateCache(s); };
    api["enable"]   = [](sol::this_state s){ initStateCache(s); };
    api["disable"]  = [](sol::this_state s){ removeStateCache(s); };
    api["enabled"]  = [](sol::this_state s){ return isStateCacheEnabled(s); };
    api["size"]     = [](sol::this_state s){ return getCacheSize(s); };
    api["capacity"] = [](sol::this_state s, const sol::stack_object& value) {
        auto previous = getCacheCapacity(s);
        if (value.valid()) {
            REQUIRE(value.get_type() == sol::type::number)
                    << "bad argument #1 to 'effil.cache.capacity' "
                       "(number expected, got " << luaTypename(value) << ")";
            REQUIRE(value.as<int>() >= 0)
                    << "effil.cache.capacity: invalid capacity value = "
                    << value.as<int>();
            setCacheCapacity(s, value.as<size_t>());
        }
        return previous;
    };
    api["remove"] = [](sol::this_state s, const sol::stack_object& value) {
        REQUIRE(value.get_type() == sol::type::function)
                << "bad argument #1 to 'effil.cache.remove' "
                   "(function expected, got " << luaTypename(value) << ")";
        return removeFromCache(s, value.as<sol::function>());
    };

    return api;
}

} // namespace cache
} // namespace effil

