#include "function.h"
#include "cache.h"

namespace effil {

namespace {

bool allowTableUpvalues(const sol::optional<bool>& newValue = sol::nullopt) {
    static std::atomic_bool value(true);

    if (newValue)
        return value.exchange(newValue.value());
    return value;
}

} // anonymous

sol::object luaAllowTableUpvalues(sol::this_state state, const sol::stack_object& value) {
    if (value.valid()) {
        REQUIRE(value.get_type() == sol::type::boolean) << "bad argument #1 to 'effil.allow_table_upvalues' (boolean expected, got " << luaTypename(value) << ")";
        return sol::make_object(state, allowTableUpvalues(value.template as<bool>()));
    }
    else {
        return sol::make_object(state, allowTableUpvalues());
    }
}

Function Function::create(const sol::function& luaFunc) {
    const auto cachedFunc = cache::get(luaFunc.lua_state(), luaFunc);
    if (cachedFunc)
        return cachedFunc.value();

    Function func = GC::instance().create<Function>(luaFunc);
    cache::put(luaFunc.lua_state(), luaFunc, func);
    cache::put(luaFunc.lua_state(), func, luaFunc);
    return func;
}

Function::Function(const sol::function& luaObject) {
    assert(luaObject.valid());
    assert(luaObject.get_type() == sol::type::function);

    sol::state_view state = luaObject.lua_state();
    sol::stack::push(state, luaObject);

    lua_Debug dbgInfo;
    lua_getinfo(state, ">u", &dbgInfo); // function is popped from stack here
    sol::stack::push(state, luaObject);

    ctx_->function = dumpFunction(luaObject);
    ctx_->upvalues.resize(dbgInfo.nups);
#if LUA_VERSION_NUM > 501
    ctx_->envUpvaluePos = 0; // means no _ENV upvalue
#endif // LUA_VERSION_NUM > 501

    for (unsigned char i = 1; i <= dbgInfo.nups; ++i) {
        const char* valueName = lua_getupvalue(state, -1, i); // push value on stack
        (void)valueName; // get rid of 'unused' warning for Lua5.1
        assert(valueName != nullptr);

#if LUA_VERSION_NUM > 501
        if (strcmp(valueName, "_ENV") == 0) { // do not serialize _ENV
            sol::stack::pop<sol::object>(state);
            ctx_->envUpvaluePos = i;
            continue;
        }
#endif // LUA_VERSION_NUM > 501

        const auto& upvalue = sol::stack::pop<sol::object>(state); // pop from stack
        if (!allowTableUpvalues() && upvalue.get_type() == sol::type::table) {
            sol::stack::pop<sol::object>(state);
            throw effil::Exception() << "bad function upvalue #" << (int)i << " (table is disabled by effil.allow_table_upvalues)";
        }

        StoredObject storedObject;
        try {
            storedObject = createStoredObject(upvalue);
            assert(storedObject.get() != nullptr);
        }
        catch(const std::exception& err) {
            sol::stack::pop<sol::object>(state);
            throw effil::Exception() << "bad function upvalue #" << (int)i << " (" << err.what() << ")";
        }

        if (storedObject->gcHandle() != nullptr) {
            ctx_->addReference(storedObject->gcHandle());
            storedObject->releaseStrongReference();
        }
        ctx_->upvalues[i - 1] = std::move(storedObject);
    }
    sol::stack::pop<sol::object>(state);
}

sol::object Function::loadFunction(lua_State* state) {
    auto cachedFunc = cache::get(state, *this);
    if (cachedFunc.valid())
        return cachedFunc;

    sol::function result = loadString(state, ctx_->function);
    assert(result.valid());

    sol::stack::push(state, result);
    for(size_t i = 0; i < ctx_->upvalues.size(); ++i) {
#if LUA_VERSION_NUM > 501
        if (ctx_->envUpvaluePos == i + 1) {
            lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS); // push _ENV to stack
            lua_setupvalue(state, -2, i + 1); // pop _ENV and set as upvalue
            continue;
        }
#endif // LUA_VERSION_NUM > 501
        assert(ctx_->upvalues[i].get() != nullptr);

        const auto& obj = ctx_->upvalues[i]->unpack(sol::this_state{state});
        sol::stack::push(state, obj);
        lua_setupvalue(state, -2, i + 1);
    }
    const auto obj = sol::stack::pop<sol::object>(state);
    cache::put(state, *this, obj);
    cache::put(state, obj, *this);
    return obj;
}

} // namespace effil
