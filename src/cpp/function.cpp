#include "function.h"

namespace effil {

FunctionData::FunctionData() : cFunction(nullptr), isCFunction(true)
{}

FunctionData::~FunctionData()
{
    if (!isCFunction)
        function.~basic_string();
}

Function::Function(const sol::function& luaObject) {
    assert(luaObject.valid());
    assert(luaObject.get_type() == sol::type::function);

    lua_State* state = luaObject.lua_state();
    sol::stack::push(state, luaObject);

    lua_Debug dbgInfo;
    lua_getinfo(state, ">u", &dbgInfo); // function is popped from stack here
    sol::stack::push(state, luaObject);

    if (lua_iscfunction(state, -1))
    {
        ctx_->isCFunction = true;
        ctx_->cFunction = lua_tocfunction (state, -1);
        REQUIRE(ctx_->cFunction) << "cannot get pointer to provided C function";
    }
    else
    {
        ctx_->isCFunction = false;
        ctx_->function = dumpFunction(luaObject);
    }

    ctx_->upvalues.resize(dbgInfo.nups);
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
            continue;
        }
#endif // LUA_VERSION_NUM > 501

        StoredObject storedObject;
        try {
            const auto& upvalue = sol::stack::pop<sol::object>(state);
            storedObject = createStoredObject(upvalue);
            assert(storedObject.get() != nullptr);
        }
        catch(const std::exception& err) {
            sol::stack::pop<sol::object>(state);
            throw effil::Exception() << "bad function upvalue #" << (int)i
                                     << " (" << err.what() << ")";
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
    if (ctx_->isCFunction)
    {
        assert(ctx_->cFunction);
        sol::stack::push(state, ctx_->cFunction);
    }
    else
    {
        sol::function func = loadString(state, ctx_->function);
        assert(func.valid());
        sol::stack::push(state, func);
    }

    for(size_t i = 0; i < ctx_->upvalues.size(); ++i) {
#if LUA_VERSION_NUM > 501
        if (ctx_->envUpvaluePos == i + 1) {
            lua_pushglobaltable(state); // push _G to stack
            lua_setupvalue(state, -2, i + 1); // pop _G and set as upvalue
            continue;
        }
#endif // LUA_VERSION_NUM > 501
        assert(ctx_->upvalues[i].get() != nullptr);

        const auto& obj = ctx_->upvalues[i]->unpack(sol::this_state{state});
        sol::stack::push(state, obj);
        lua_setupvalue(state, -2, i + 1);
    }
    return sol::stack::pop<sol::function>(state);
}

} // namespace effil
