#pragma once

#include "gc-object.h"
#include "stored-object.h"
#include "spin-mutex.h"
#include "utils.h"
#include "lua-helpers.h"

#include <sol.hpp>

#include <map>
#include <memory>

namespace effil {

class SharedTable : public GCObject {
private:
    typedef std::pair<sol::object, sol::object> PairsIterator;
    typedef std::map<StoredObject, StoredObject, StoredObjectLess> DataEntries;

public:
    SharedTable();
    SharedTable& operator=(const SharedTable&) = default;

    static void exportAPI(sol::state_view& lua);

    void set(StoredObject&&, StoredObject&&);
    void rawSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue);
    sol::object get(const StoredObject& key, sol::this_state state) const;
    sol::object rawGet(const sol::stack_object& key, sol::this_state state) const;
    static sol::object basicMetaMethod(const std::string&, const std::string&, sol::this_state,
                                        const sol::stack_object&, const sol::stack_object&);

    // These functions are metamethods available in Lua
    void luaNewIndex(const sol::stack_object& luaKey, const sol::stack_object& luaValue, sol::this_state);
    sol::object luaIndex(const sol::stack_object& key, sol::this_state state);
    sol::object luaToString(sol::this_state state);
    sol::object luaLength(sol::this_state state);
    PairsIterator luaPairs(sol::this_state);
    PairsIterator luaIPairs(sol::this_state);
    StoredArray luaCall(sol::this_state state, const sol::variadic_args& args);
    sol::object luaUnm(sol::this_state);
    static sol::object luaAdd(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaSub(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaMul(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaDiv(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaMod(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaPow(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaEq(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaLe(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaLt(sol::this_state, const sol::stack_object&, const sol::stack_object&);
    static sol::object luaConcat(sol::this_state, const sol::stack_object&, const sol::stack_object&);

    // Stand alone functions for effil::table available in Lua
    static SharedTable luaSetMetatable(SharedTable& stable, const sol::stack_object& mt);
    static sol::object luaGetMetatable(const SharedTable& stable, const sol::this_state& state);
    static sol::object luaRawGet(const SharedTable& stable, const sol::stack_object& key, sol::this_state state);
    static SharedTable luaRawSet(SharedTable& stable, const sol::stack_object& key, const sol::stack_object& value);
    static size_t luaSize(SharedTable& stable);
    static PairsIterator globalLuaPairs(sol::this_state state, SharedTable& obj);
    static PairsIterator globalLuaIPairs(sol::this_state state, SharedTable& obj);

private:
    PairsIterator getNext(const sol::object& key, sol::this_state lua);

private:
    struct SharedData {
        SpinMutex lock;
        DataEntries entries;
        GCObjectHandle metatable;
        std::unordered_set<GCObjectHandle> refs_;

        SharedData() : metatable(GCNull) {}
    };

    std::shared_ptr<SharedData> data_;
};

} // effil
