#include "shared-table.h"

#include "utils.h"

#include <mutex>
#include <cassert>

namespace effil {

namespace {

template<typename SolObject>
bool isSharedTable(const SolObject& obj) {
    return obj.valid() && obj.get_type() == sol::type::userdata && obj.template is<SharedTableView>();
}

template<typename SolObject>
bool isAnyTable(const SolObject& obj) {
    return obj.valid() && ((obj.get_type() == sol::type::userdata && obj.template is<SharedTableView>()) || obj.get_type() == sol::type::table);
}

} // namespace

void SharedTableView::exportAPI(sol::state_view& lua) {
    sol::usertype<SharedTableView> type("new", sol::no_constructor,
        "__pairs",  &SharedTableView::luaPairs,
        "__ipairs", &SharedTableView::luaIPairs,
        sol::meta_function::new_index,              &SharedTableView::luaNewIndex,
        sol::meta_function::index,                  &SharedTableView::luaIndex,
        sol::meta_function::length,                 &SharedTableView::luaLength,
        sol::meta_function::to_string,              &SharedTableView::luaToString,
        sol::meta_function::addition,               &SharedTableView::luaAdd,
        sol::meta_function::subtraction,            &SharedTableView::luaSub,
        sol::meta_function::multiplication,         &SharedTableView::luaMul,
        sol::meta_function::division,               &SharedTableView::luaDiv,
        sol::meta_function::modulus,                &SharedTableView::luaMod,
        sol::meta_function::power_of,               &SharedTableView::luaPow,
        sol::meta_function::concatenation,          &SharedTableView::luaConcat,
        sol::meta_function::less_than,              &SharedTableView::luaLt,
        sol::meta_function::unary_minus,            &SharedTableView::luaUnm,
        sol::meta_function::call,                   &SharedTableView::luaCall,
        sol::meta_function::equal_to,               &SharedTableView::luaEq,
        sol::meta_function::less_than_or_equal_to,  &SharedTableView::luaLe
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

void SharedTableView::set(StoredObject&& key, StoredObject&& value) {
    std::lock_guard<SpinMutex> g(impl_->lock);

    impl_->addReference(key->gcHandle());
    impl_->addReference(value->gcHandle());

    key->releaseStrongReference();
    value->releaseStrongReference();

    impl_->entries[std::move(key)] = std::move(value);
}

sol::object SharedTableView::get(const StoredObject& key, sol::this_state state) const {
    std::lock_guard<SpinMutex> g(impl_->lock);
    auto val = impl_->entries.find(key);
    if (val == impl_->entries.end()) {
        return sol::nil;
    } else {
        return val->second->unpack(state);
    }
}

void SharedTableView::rawSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue) {
    REQUIRE(luaKey.valid()) << "Indexing by nil";

    StoredObject key = createStoredObject(luaKey);
    if (luaValue.get_type() == sol::type::nil) {
        std::lock_guard<SpinMutex> g(impl_->lock);

        // in this case object is not obligatory to own data
        auto it = impl_->entries.find(key);
        if (it != impl_->entries.end()) {
            impl_->removeReference(it->first->gcHandle());
            impl_->removeReference(it->second->gcHandle());
            impl_->entries.erase(it);
        }

    } else {
        set(std::move(key), createStoredObject(luaValue));
    }
}

sol::object SharedTableView::rawGet(const sol::stack_object& luaKey, sol::this_state state) const {
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
        std::unique_lock<SpinMutex> lock(impl_->lock); \
        if (impl_->metatable != GCNull) { \
            auto tableHolder = GC::instance().get<SharedTableView>(impl_->metatable); \
            lock.unlock(); \
            sol::function handler = tableHolder.get(createStoredObject(methodName), state); \
            if (handler.valid()) { \
                return handler(__VA_ARGS__); \
            } \
        } \
    }

#define PROXY_METAMETHOD_IMPL(tableMethod, methodName, errMsg) \
    sol::object SharedTableView:: tableMethod(sol::this_state state, \
            const sol::stack_object& leftObject, const sol::stack_object& rightObject) { \
        return basicMetaMethod(methodName, errMsg, state, leftObject, rightObject); \
    }

namespace {
const std::string ARITHMETIC_ERR_MSG = "attempt to perform arithmetic on a effil::table value";
const std::string COMPARE_ERR_MSG = "attempt to compare a effil::table value";
const std::string CONCAT_ERR_MSG = "attempt to concatenate a effil::table value";
}

sol::object SharedTableView::basicMetaMethod(const std::string& metamethodName, const std::string& errMsg,
            sol::this_state state, const sol::stack_object& leftObject, const sol::stack_object& rightObject) {
    if (isSharedTable(leftObject)) {
        SharedTableView table = leftObject.as<SharedTableView>();
        auto impl_ = table.impl_;
        DEFFINE_METAMETHOD_CALL(metamethodName, table, rightObject)
    }
    if (isSharedTable(rightObject)) {
        SharedTableView table = rightObject.as<SharedTableView>();
        auto impl_ = table.impl_;
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

sol::object SharedTableView::luaUnm(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__unm")
    throw Exception() << ARITHMETIC_ERR_MSG;
}

void SharedTableView::luaNewIndex(const sol::stack_object& luaKey, const sol::stack_object& luaValue, sol::this_state state) {
    {
        std::unique_lock<SpinMutex> lock(impl_->lock);
        if (impl_->metatable != GCNull) {
            auto tableHolder = GC::instance().get<SharedTableView>(impl_->metatable);
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

sol::object SharedTableView::luaIndex(const sol::stack_object& luaKey, sol::this_state state) {
    DEFFINE_METAMETHOD_CALL("__index", *this, luaKey)
    try {
        return rawGet(luaKey, state);
    } RETHROW_WITH_PREFIX("effil.table");
}

StoredArray SharedTableView::luaCall(sol::this_state state, const sol::variadic_args& args) {
    std::unique_lock<SpinMutex> lock(impl_->lock);
    if (impl_->metatable != GCNull) {
        auto metatable = GC::instance().get<SharedTableView>(impl_->metatable);
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

sol::object SharedTableView::luaToString(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__tostring");
    std::stringstream ss;
    ss << "effil.table: " << impl_.get();
    return sol::make_object(state, ss.str());
}

sol::object SharedTableView::luaLength(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__len");
    std::lock_guard<SpinMutex> g(impl_->lock);
    size_t len = 0u;
    sol::optional<LUA_INDEX_TYPE> value;
    auto iter = impl_->entries.find(createStoredObject(static_cast<LUA_INDEX_TYPE>(1)));
    if (iter != impl_->entries.end()) {
        do {
            ++len;
            ++iter;
        } while ((iter != impl_->entries.end()) && (value = storedObjectToIndexType(iter->first)) &&
                 (static_cast<size_t>(value.value()) == len + 1));
    }
    return sol::make_object(state, len);
}

SharedTableView::PairsIterator SharedTableView::getNext(const sol::object& key, sol::this_state lua) {
    std::lock_guard<SpinMutex> g(impl_->lock);
    if (key) {
        auto obj = createStoredObject(key);
        auto upper = impl_->entries.upper_bound(obj);
        if (upper != impl_->entries.end())
            return PairsIterator(upper->first->unpack(lua), upper->second->unpack(lua));
    } else {
        if (!impl_->entries.empty()) {
            const auto& begin = impl_->entries.begin();
            return PairsIterator(begin->first->unpack(lua), begin->second->unpack(lua));
        }
    }
    return PairsIterator(sol::nil, sol::nil);
}

SharedTableView::PairsIterator SharedTableView::luaPairs(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__pairs");
    auto next = [](sol::this_state state, SharedTableView table, sol::stack_object key) { return table.getNext(key, state); };
    return PairsIterator(
        sol::make_object(state, std::function<PairsIterator(sol::this_state state, SharedTableView table, sol::stack_object key)>(next)).as<sol::function>(),
        sol::make_object(state, *this));
}

std::pair<sol::object, sol::object> ipairsNext(sol::this_state lua, SharedTableView table, const sol::optional<LUA_INDEX_TYPE>& key) {
    size_t index = key ? key.value() + 1 : 1;
    auto objKey = createStoredObject(static_cast<LUA_INDEX_TYPE>(index));
    sol::object value = table.get(objKey, lua);
    if (!value.valid())
        return std::pair<sol::object, sol::object>(sol::nil, sol::nil);
    return std::pair<sol::object, sol::object>(objKey->unpack(lua), value);
}

SharedTableView::PairsIterator SharedTableView::luaIPairs(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__ipairs");
    return PairsIterator(sol::make_object(state, ipairsNext).as<sol::function>(),
                sol::make_object(state, *this));
}

/*
 * Lua static API functions
 */

SharedTableView SharedTableView::luaSetMetatable(const sol::stack_object& tbl, const sol::stack_object& mt) {
    REQUIRE(isAnyTable(tbl)) << "bad argument #1 to 'effil.setmetatable' (table expected, got " << luaTypename(tbl) << ")";
    REQUIRE(isAnyTable(mt)) << "bad argument #2 to 'effil.setmetatable' (table expected, got " << luaTypename(mt) << ")";

    SharedTableView stable = GC::instance().get<SharedTableView>(createStoredObject(tbl)->gcHandle());

    std::lock_guard<SpinMutex> lock(stable.impl_->lock);
    if (stable.impl_->metatable != GCNull) {
        stable.impl_->removeReference(stable.impl_->metatable);
        stable.impl_->metatable = GCNull;
    }

    stable.impl_->metatable = createStoredObject(mt)->gcHandle();
    stable.impl_->addReference(stable.impl_->metatable);

    return stable;
}

sol::object SharedTableView::luaGetMetatable(const sol::stack_object& tbl, sol::this_state state) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.getmetatable' (effil.table expected, got " << luaTypename(tbl) << ")";
    auto& stable = tbl.as<SharedTableView>();

    std::lock_guard<SpinMutex> lock(stable.impl_->lock);
    return stable.impl_->metatable == GCNull ? sol::nil :
            sol::make_object(state, GC::instance().get<SharedTableView>(stable.impl_->metatable));
}

sol::object SharedTableView::luaRawGet(const sol::stack_object& tbl, const sol::stack_object& key, sol::this_state state) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.rawget' (effil.table expected, got " << luaTypename(tbl) << ")";
    try {
        return tbl.as<SharedTableView>().rawGet(key, state);
    } RETHROW_WITH_PREFIX("effil.rawget");
}

SharedTableView SharedTableView::luaRawSet(const sol::stack_object& tbl, const sol::stack_object& key, const sol::stack_object& value) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.rawset' (effil.table expected, got " << luaTypename(tbl) << ")";
    try {
        auto& stable = tbl.as<SharedTableView>();
        stable.rawSet(key, value);
        return stable;
    } RETHROW_WITH_PREFIX("effil.rawset");
}

size_t SharedTableView::luaSize(const sol::stack_object& tbl) {
    REQUIRE(isSharedTable(tbl)) << "bad argument #1 to 'effil.size' (effil.table expected, got " << luaTypename(tbl) << ")";
    try {
        auto& stable = tbl.as<SharedTableView>();
        std::lock_guard<SpinMutex> g(stable.impl_->lock);
        return stable.impl_->entries.size();
    } RETHROW_WITH_PREFIX("effil.size");
}

SharedTableView::PairsIterator SharedTableView::globalLuaPairs(sol::this_state state, const sol::stack_object& obj) {
    REQUIRE(isSharedTable(obj)) << "bad argument #1 to 'effil.pairs' (effil.table expected, got " << luaTypename(obj) << ")";
    auto& tbl = obj.as<SharedTableView>();
    return tbl.luaPairs(state);
}

SharedTableView::PairsIterator SharedTableView::globalLuaIPairs(sol::this_state state, const sol::stack_object& obj) {
    REQUIRE(isSharedTable(obj)) << "bad argument #1 to 'effil.ipairs' (effil.table expected, got " << luaTypename(obj) << ")";
    auto& tbl = obj.as<SharedTableView>();
    return tbl.luaIPairs(state);
}

#undef DEFFINE_METAMETHOD_CALL_0
#undef DEFFINE_METAMETHOD_CALL
#undef PROXY_METAMETHOD_IMPL

} // effil
