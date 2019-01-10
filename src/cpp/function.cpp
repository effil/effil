#include "function.h"

namespace effil {

Function::Function(const sol::function& luaObject) {
    assert(luaObject.valid());
    assert(luaObject.get_type() == sol::type::function);

    lua_State* state = luaObject.lua_state();
    sol::stack::push(state, luaObject);

    lua_Debug dbgInfo;
    lua_getinfo(state, ">u", &dbgInfo); // function is popped from stack here
    sol::stack::push(state, luaObject);

    ctx_->function = dumpFunction(luaObject);
#if LUA_VERSION_NUM > 501
    ctx_->envUpvaluePos = 0; // means no _G upvalue

    lua_pushglobaltable(state);
    const auto gTable = sol::stack::pop<sol::table>(state);
#endif // LUA_VERSION_NUM > 501


    for (unsigned char i = 1; i <= dbgInfo.nups; ++i) {
        const char* valueName = lua_getupvalue(state, -1, i); // push value on stack
        (void)valueName; // get rid of 'unused' warning for Lua5.1
        assert(valueName != nullptr);

#if LUA_VERSION_NUM > 501
        if (gTable == sol::stack::get<sol::table>(state)) { // do not serialize _G
            sol::stack::pop<sol::object>(state);
            ctx_->envUpvaluePos = i;
            ctx_->upvalues.emplace_back(StoredObject(sol::nil));
            continue;
        }
#endif // LUA_VERSION_NUM > 501

        try {
            const auto& upvalue = sol::stack::pop<sol::object>(state);
            auto storedObject = createStoredObject(upvalue);

            if (storedObject.gcHandle() != nullptr) {
                ctx_->addReference(storedObject.gcHandle());
                storedObject.releaseStrongReference();
            }
            ctx_->upvalues.emplace_back(std::move(storedObject));
        }
        catch(const std::exception& err) {
            sol::stack::pop<sol::object>(state);
            throw effil::Exception() << "bad function upvalue #" << (int)i << " (" << err.what() << ")";
        }

    }
    sol::stack::pop<sol::object>(state);
}

sol::object Function::loadFunction(lua_State* state) {
    sol::function result = loadString(state, ctx_->function);
    assert(result.valid());

    sol::stack::push(state, result);
    for(size_t i = 0; i < ctx_->upvalues.size(); ++i) {
#if LUA_VERSION_NUM > 501
        if (ctx_->envUpvaluePos == i + 1) {
            lua_pushglobaltable(state); // push _G to stack
            lua_setupvalue(state, -2, i + 1); // pop _G and set as upvalue
            continue;
        }
#endif // LUA_VERSION_NUM > 501

        const auto& obj = ctx_->upvalues[i].unpack(sol::this_state{state});
        sol::stack::push(state, obj);
        lua_setupvalue(state, -2, i + 1);
    }
    return sol::stack::pop<sol::function>(state);
}

} // namespace effil
