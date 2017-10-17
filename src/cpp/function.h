#pragma once

#include "gc-object.h"
#include "utils.h"
#include "lua-helpers.h"

namespace effil {

sol::object luaAllowTableUpvalues(sol::this_state state, const sol::stack_object&);

class FunctionObject: public GCObject {
public:
    template <typename SolType>
    FunctionObject(const SolType& luaObject)
            : data_(std::make_shared<SharedData>()) {
        initialize(luaObject);
    }

    sol::object loadFunction(lua_State* state);

private:
    void initialize(const sol::function& luaObject);

    struct SharedData {
        std::string function;
#if LUA_VERSION_NUM > 501
        unsigned char envUpvaluePos;
#endif // LUA_VERSION_NUM > 501
        std::vector<StoredObject> upvalues;
    };

    std::shared_ptr<SharedData> data_;
};

} // namespace effil
