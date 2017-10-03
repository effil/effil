#include "function.h"

namespace effil {

namespace {

bool allow_table_upvalue(const sol::optional<bool>& newValue = sol::nullopt) {
    static std::mutex mutex;
    static bool value = true;

    std::unique_lock<std::mutex> lock(mutex);
    bool oldValue = value;
    if (newValue)
        value = newValue.value();
    return oldValue;
}

} // anonymous

sol::object lua_allow_table_upvalue(sol::this_state state, const sol::stack_object& value) {
    if (value.valid()) {
        REQUIRE(value.get_type() == sol::type::boolean) << "bad argument #1 to 'effil.allow_table_upvalue' (boolean expected, got " << luaTypename(value) << ")";
        return sol::make_object(state, allow_table_upvalue(value.template as<bool>()));
    }
    else {
        return sol::make_object(state, allow_table_upvalue());
    }
}

void FunctionObject::initialize(const sol::function& luaObject) {
    assert(luaObject.valid());
    assert(luaObject.get_type() == sol::type::function);

    lua_State* state = luaObject.lua_state();
    sol::stack::push(state, luaObject);

    lua_Debug dbgInfo;
    lua_getinfo(state, ">u", &dbgInfo); // function is popped from stack here
    sol::stack::push(state, luaObject);

    data_->function = dumpFunction(luaObject);
    data_->upvalues.resize(dbgInfo.nups);
    data_->envUpvaluePos = 0; // means no _ENV upvalue

    for (unsigned char i = 1; i <= dbgInfo.nups; ++i) {
        const char* valueName = lua_getupvalue(state, -1, i); // push value on stack
        assert(valueName != nullptr);

        if (strcmp(valueName, "_ENV") == 0) { // do not serialize _ENV
            sol::stack::pop<sol::object>(state);
            data_->envUpvaluePos = i;
            continue;
        }

        const auto& upvalue = sol::stack::pop<sol::object>(state); // pop from stack
        if (!allow_table_upvalue() && upvalue.get_type() == sol::type::table) {
            sol::stack::pop<sol::object>(state);
            throw effil::Exception() << "bad function upvalue #" << (int)i << " (table is disabled by effil.allow_table_upvalue)";
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
            addReference(storedObject->gcHandle());
            storedObject->releaseStrongReference();
        }
        data_->upvalues[i - 1] = std::move(storedObject);
    }
    sol::stack::pop<sol::object>(state);
}

sol::object FunctionObject::loadFunction(lua_State* state) {
    sol::function result = loadString(state, data_->function);
    assert(result.valid());

    sol::stack::push(state, result);
    for(size_t i = 0; i < data_->upvalues.size(); ++i) {
        if (data_->envUpvaluePos == i + 1) {
            lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS); // push _ENV to stack
            lua_setupvalue(state, -2, i + 1); // pop _ENV and set as upvalue
            continue;
        }
        assert(data_->upvalues[i].get() != nullptr);

        const auto& obj = data_->upvalues[i]->unpack(sol::this_state{state});
        sol::stack::push(state, obj);
        lua_setupvalue(state, -2, i + 1);
    }
    return sol::stack::pop<sol::function>(state);
}

} // namespace effil
