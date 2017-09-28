#include "shared-table.h"

#include "utils.h"

#include <mutex>
#include <cassert>

namespace effil {

namespace {

template<typename SolObject>
bool isSharedTable(const SolObject& obj) {
    return obj.valid() && obj.get_type() == sol::type::userdata && obj.template is<SharedTable>();
}

template<typename SolObject>
bool isAnyTable(const SolObject& obj) {
    return obj.valid() && ((obj.get_type() == sol::type::userdata && obj.template is<SharedTable>()) || obj.get_type() == sol::type::table);
}

} // namespace

SharedTable::SharedTable() : data_(std::make_shared<SharedData>()) {}

void SharedTable::exportAPI(sol::state_view& lua) {
    sol::usertype<SharedTable> type("new", sol::no_constructor,
        "__pairs",  &SharedTable::luaPairs,
        "__ipairs", &SharedTable::luaIPairs,
        sol::meta_function::new_index,              &SharedTable::luaNewIndex,
        sol::meta_function::index,                  &SharedTable::luaIndex,
        sol::meta_function::length,                 &SharedTable::luaLength,
        sol::meta_function::to_string,              &SharedTable::luaToString,
        sol::meta_function::addition,               &SharedTable::luaAdd,
        sol::meta_function::subtraction,            &SharedTable::luaSub,
        sol::meta_function::multiplication,         &SharedTable::luaMul,
        sol::meta_function::division,               &SharedTable::luaDiv,
        sol::meta_function::modulus,                &SharedTable::luaMod,
        sol::meta_function::power_of,               &SharedTable::luaPow,
        sol::meta_function::concatenation,          &SharedTable::luaConcat,
        sol::meta_function::less_than,              &SharedTable::luaLt,
        sol::meta_function::unary_minus,            &SharedTable::luaUnm,
        sol::meta_function::call,                   &SharedTable::luaCall,
        sol::meta_function::equal_to,               &SharedTable::luaEq,
        sol::meta_function::less_than_or_equal_to,  &SharedTable::luaLe
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

void SharedTable::set(StoredObject&& key, StoredObject&& value) {
    std::lock_guard<SpinMutex> g(data_->lock);

    addReference(key->gcHandle());
    addReference(value->gcHandle());

    key->releaseStrongReference();
    value->releaseStrongReference();

    data_->entries[std::move(key)] = std::move(value);
}

sol::object SharedTable::get(const StoredObject& key, sol::this_state state) const {
    std::lock_guard<SpinMutex> g(data_->lock);
    auto val = data_->entries.find(key);
    if (val == data_->entries.end()) {
        return sol::nil;
    } else {
        return val->second->unpack(state);
    }
}

void SharedTable::rawSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue) {
    REQUIRE(luaKey.valid()) << "Indexing by nil";

    StoredObject key = createStoredObject(luaKey);
    if (luaValue.get_type() == sol::type::nil) {
        std::lock_guard<SpinMutex> g(data_->lock);

        // in this case object is not obligatory to own data
        auto it = data_->entries.find(key);
        if (it != data_->entries.end()) {
            removeReference(it->first->gcHandle());
            removeReference(it->second->gcHandle());
            data_->entries.erase(it);
        }

    } else {
        set(std::move(key), createStoredObject(luaValue));
    }
}

sol::object SharedTable::rawGet(const sol::stack_object& luaKey, sol::this_state state) const {
    REQUIRE(luaKey.valid()) << "Indexing by nil";
    StoredObject key = createStoredObject(luaKey);
    return get(key, state);
}

/*
 * Lua Meta API methods
 */
#define DEFFINE_METAMETHOD_CALL_0(methodName) DEFFINE_METAMETHOD_CALL(methodName, *this)
#define DEFFINE_METAMETHOD_CALL(methodName, ...) \
    { \
        std::unique_lock<SpinMutex> lock(data_->lock); \
        if (data_->metatable != GCNull) { \
            auto tableHolder = GC::instance().get<SharedTable>(data_->metatable); \
            lock.unlock(); \
            sol::function handler = tableHolder.get(createStoredObject(methodName), state); \
            if (handler.valid()) { \
                return handler(__VA_ARGS__); \
            } \
        } \
    }

#define PROXY_METAMETHOD_IMPL(tableMethod, methodName, errMsg) \
    sol::object SharedTable:: tableMethod(sol::this_state state, \
            const sol::stack_object& leftObject, const sol::stack_object& rightObject) { \
        return basicMetaMethod(methodName, errMsg, state, leftObject, rightObject); \
    }

namespace {
const std::string ARITHMETIC_ERR_MSG = "attempt to perform arithmetic on a effil::table value";
const std::string COMPARE_ERR_MSG = "attempt to compare a effil::table value";
const std::string CONCAT_ERR_MSG = "attempt to concatenate a effil::table value";
}

sol::object SharedTable::basicMetaMethod(const std::string& metamethodName, const std::string& errMsg,
            sol::this_state state, const sol::stack_object& leftObject, const sol::stack_object& rightObject) {
    if (isSharedTable(leftObject)) {
        SharedTable table = leftObject.as<SharedTable>();
        auto data_ = table.data_;
        DEFFINE_METAMETHOD_CALL(metamethodName, table, rightObject)
    }
    if (isSharedTable(rightObject)) {
        SharedTable table = rightObject.as<SharedTable>();
        auto data_ = table.data_;
        DEFFINE_METAMETHOD_CALL(metamethodName, leftObject, table)
    }
    throw Exception() << errMsg;
}

PROXY_METAMETHOD_IMPL(luaConcat, "__concat", CONCAT_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaAdd, "__add", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaSub, "__sub", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaMul, "__mul", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaDiv, "__div", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaMod, "__mod", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaPow, "__pow", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaLe, "__le", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaLt, "__lt", ARITHMETIC_ERR_MSG)
PROXY_METAMETHOD_IMPL(luaEq, "__eq", ARITHMETIC_ERR_MSG)

sol::object SharedTable::luaUnm(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__unm")
    throw Exception() << ARITHMETIC_ERR_MSG;
}

void SharedTable::luaNewIndex(const sol::stack_object& luaKey, const sol::stack_object& luaValue, sol::this_state state) {
    {
        std::unique_lock<SpinMutex> lock(data_->lock);
        if (data_->metatable != GCNull) {
            auto tableHolder = GC::instance().get<SharedTable>(data_->metatable);
            lock.unlock();
            sol::function handler = tableHolder.get(createStoredObject("__newindex"), state);
            if (handler.valid()) {
                handler(*this, luaKey, luaValue);
                return;
            }
        }
    }
    try {
        rawSet(luaKey, luaValue);
    } RETHROW_WITH_PREFIX("effil.table");
}

sol::object SharedTable::luaIndex(const sol::stack_object& luaKey, sol::this_state state) {
    DEFFINE_METAMETHOD_CALL("__index", *this, luaKey)
    try {
        return rawGet(luaKey, state);
    } RETHROW_WITH_PREFIX("effil.table");
}

StoredArray SharedTable::luaCall(sol::this_state state, const sol::variadic_args& args) {
    std::unique_lock<SpinMutex> lock(data_->lock);
    if (data_->metatable != GCNull) {
        auto metatable = GC::instance().get<SharedTable>(data_->metatable);
        sol::function handler = metatable.get(createStoredObject(std::string("__call")), state);
        lock.unlock();
        if (handler.valid()) {
            StoredArray storedResults;
            const int savedStackTop = lua_gettop(state);
            sol::function_result callResults = handler(*this, args);
            (void)callResults;
            sol::variadic_args funcReturns(state, savedStackTop - lua_gettop(state));
            for (const auto& param : funcReturns)
                storedResults.emplace_back(createStoredObject(param.get<sol::object>()));
            return storedResults;
        }
    }
    throw Exception() << "attempt to call a table";
}

sol::object SharedTable::luaToString(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__tostring");
    std::stringstream ss;
    ss << "effil.table: " << data_.get();
    return sol::make_object(state, ss.str());
}

sol::object SharedTable::luaLength(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__len");
    std::lock_guard<SpinMutex> g(data_->lock);
    size_t len = 0u;
    sol::optional<LUA_INDEX_TYPE> value;
    auto iter = data_->entries.find(createStoredObject(static_cast<LUA_INDEX_TYPE>(1)));
    if (iter != data_->entries.end()) {
        do {
            ++len;
            ++iter;
        } while ((iter != data_->entries.end()) && (value = storedObjectToIndexType(iter->first)) &&
                 (static_cast<size_t>(value.value()) == len + 1));
    }
    return sol::make_object(state, len);
}

SharedTable::PairsIterator SharedTable::getNext(const sol::object& key, sol::this_state lua) {
    std::lock_guard<SpinMutex> g(data_->lock);
    if (key) {
        auto obj = createStoredObject(key);
        auto upper = data_->entries.upper_bound(obj);
        if (upper != data_->entries.end())
            return PairsIterator(upper->first->unpack(lua), upper->second->unpack(lua));
    } else {
        if (!data_->entries.empty()) {
            const auto& begin = data_->entries.begin();
            return PairsIterator(begin->first->unpack(lua), begin->second->unpack(lua));
        }
    }
    return PairsIterator(sol::nil, sol::nil);
}

SharedTable::PairsIterator SharedTable::luaPairs(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__pairs");
    auto next = [](sol::this_state state, SharedTable table, sol::stack_object key) { return table.getNext(key, state); };
    return PairsIterator(
        sol::make_object(state, std::function<PairsIterator(sol::this_state state, SharedTable table, sol::stack_object key)>(next)).as<sol::function>(),
        sol::make_object(state, *this));
}

std::pair<sol::object, sol::object> ipairsNext(sol::this_state lua, SharedTable table, const sol::optional<LUA_INDEX_TYPE>& key) {
    size_t index = key ? key.value() + 1 : 1;
    auto objKey = createStoredObject(static_cast<LUA_INDEX_TYPE>(index));
    sol::object value = table.get(objKey, lua);
    if (!value.valid())
        return std::pair<sol::object, sol::object>(sol::nil, sol::nil);
    return std::pair<sol::object, sol::object>(objKey->unpack(lua), value);
}

SharedTable::PairsIterator SharedTable::luaIPairs(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__ipairs");
    return PairsIterator(sol::make_object(state, ipairsNext).as<sol::function>(),
                sol::make_object(state, *this));
}

/*
 * Lua static API functions
 */

SharedTable SharedTable::luaSetMetatable(const sol::stack_object& tbl, const sol::stack_object& mt) {
    REQUIRE(isAnyTable(tbl)) << "bad argument #1 to 'effil.setmetatable' (table expected, got " << luaTypename(tbl) << ")";
    REQUIRE(isAnyTable(mt)) << "bad argument #2 to 'effil.setmetatable' (table expected, got " << luaTypename(mt) << ")";

    SharedTable stable = GC::instance().get<SharedTable>(createStoredObject(tbl)->gcHandle());

    std::lock_guard<SpinMutex> lock(stable.data_->lock);
    if (stable.data_->metatable != GCNull) {
        stable.removeReference(stable.data_->metatable);
        stable.data_->metatable = GCNull;
    }

    stable.data_->metatable = createStoredObject(mt)->gcHandle();
    stable.addReference(stable.data_->metatable);

    return stable;
}

sol::object SharedTable::luaGetMetatable(const sol::stack_object& tbl, sol::this_state state) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.getmetatable' (effil.table expected, got " << luaTypename(tbl) << ")";
    auto& stable = tbl.as<SharedTable>();

    std::lock_guard<SpinMutex> lock(stable.data_->lock);
    return stable.data_->metatable == GCNull ? sol::nil :
            sol::make_object(state, GC::instance().get<SharedTable>(stable.data_->metatable));
}

sol::object SharedTable::luaRawGet(const sol::stack_object& tbl, const sol::stack_object& key, sol::this_state state) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.rawget' (effil.table expected, got " << luaTypename(tbl) << ")";
    try {
        return tbl.as<SharedTable>().rawGet(key, state);
    } RETHROW_WITH_PREFIX("effil.rawget");
}

SharedTable SharedTable::luaRawSet(const sol::stack_object& tbl, const sol::stack_object& key, const sol::stack_object& value) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.rawset' (effil.table expected, got " << luaTypename(tbl) << ")";
    try {
        auto& stable = tbl.as<SharedTable>();
        stable.rawSet(key, value);
        return stable;
    } RETHROW_WITH_PREFIX("effil.rawset");
}

size_t SharedTable::luaSize(const sol::stack_object& tbl) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.size' (effil.table expected, got " << luaTypename(tbl) << ")";
    try {
        auto& stable = tbl.as<SharedTable>();
        std::lock_guard<SpinMutex> g(stable.data_->lock);
        return stable.data_->entries.size();
    } RETHROW_WITH_PREFIX("effil.size");
}

SharedTable::PairsIterator SharedTable::globalLuaPairs(sol::this_state state, const sol::stack_object& obj) {
    REQUIRE(isSharedTable(obj)) << "bad argument #1 to 'effil.pairs' (effil.table expected, got " << luaTypename(obj) << ")";
    auto& tbl = obj.as<SharedTable>();
    return tbl.luaPairs(state);
}

SharedTable::PairsIterator SharedTable::globalLuaIPairs(sol::this_state state, const sol::stack_object& obj) {
    REQUIRE(isSharedTable(obj)) << "bad argument #1 to 'effil.ipairs' (effil.table expected, got " << luaTypename(obj) << ")";
    auto& tbl = obj.as<SharedTable>();
    return tbl.luaIPairs(state);
}

#undef DEFFINE_METAMETHOD_CALL_0
#undef DEFFINE_METAMETHOD_CALL
#undef PROXY_METAMETHOD_IMPL

} // effil
