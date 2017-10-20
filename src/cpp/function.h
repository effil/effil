#pragma once

#include "impl.h"
#include "utils.h"
#include "lua-helpers.h"
#include "view.h"

namespace effil {

sol::object luaAllowTableUpvalues(sol::this_state state, const sol::stack_object&);

class FunctionImpl : public BaseImpl {
public:
    std::string function;
#if LUA_VERSION_NUM > 501
    unsigned char envUpvaluePos;
#endif // LUA_VERSION_NUM > 501
    std::vector<StoredObject> upvalues;
};

class FunctionView : public View<FunctionImpl> {
public:
    sol::object loadFunction(lua_State* state);

private:
    explicit FunctionView(const sol::function& luaObject);
    friend class GC;
};

} // namespace effil
