#pragma once

#include "gc-data.h"
#include "utils.h"
#include "lua-helpers.h"
#include "gc-object.h"

namespace effil {

class FunctionData : public GCData {
public:
    std::string function;
#if LUA_VERSION_NUM > 501
    unsigned char envUpvaluePos;
#endif // LUA_VERSION_NUM > 501
    std::vector<StoredObject> upvalues;
};

class Function : public GCObject<FunctionData> {
public:
    sol::object loadFunction(lua_State* state);

private:
    explicit Function(const sol::function& luaObject);
    friend class GC;
};

} // namespace effil
