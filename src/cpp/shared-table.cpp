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

}

SharedTable::SharedTable() : GCObject(), data_(std::make_shared<SharedData>()) {}

SharedTable::SharedTable(const SharedTable& init)
        : GCObject(init)
        , data_(init.data_) {}

void SharedTable::getUserType(sol::state_view& lua) {
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

    if (key->gcHandle())
        refs_->insert(key->gcHandle());
    if (value->gcHandle())
        refs_->insert(value->gcHandle());

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
            if (it->first->gcHandle())
                refs_->erase(it->first->gcHandle());
            if (it->second->gcHandle())
                refs_->erase(it->second->gcHandle());
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
            SharedTable tableHolder = *static_cast<SharedTable*>(getGC().get(data_->metatable)); \
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
            SharedTable tableHolder = *static_cast<SharedTable*>(getGC().get(data_->metatable));
            lock.unlock();
            sol::function handler = tableHolder.get(createStoredObject("__newindex"), state);
            if (handler.valid()) {
                handler(*this, luaKey, luaValue);
                return;
            }
        }
    }
    rawSet(luaKey, luaValue);
}

sol::object SharedTable::luaIndex(const sol::stack_object& luaKey, sol::this_state state) {
    DEFFINE_METAMETHOD_CALL("__index", *this, luaKey)
    return rawGet(luaKey, state);
}

MultipleReturn SharedTable::luaCall(sol::this_state state, const sol::variadic_args& args) {
    std::unique_lock<SpinMutex> lock(data_->lock);
    if (data_->metatable != GCNull) {
        sol::function handler = static_cast<SharedTable*>(getGC().get(data_->metatable))->get(createStoredObject(std::string("__call")), state);
        lock.unlock();
        if (handler.valid()) {
            MultipleReturn storedResults;
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
    ss << "effil::table (0x" << std::hex << this << ")";
    return sol::make_object(state, ss.str());
}

sol::object SharedTable::luaLength(sol::this_state state) {
    DEFFINE_METAMETHOD_CALL_0("__len");
    std::lock_guard<SpinMutex> g(data_->lock);
    size_t len = 0u;
    sol::optional<double> value;
    auto iter = data_->entries.find(createStoredObject(static_cast<double>(1)));
    if (iter != data_->entries.end()) {
        do {
            ++len;
            ++iter;
        } while ((iter != data_->entries.end()) && (value = storedObjectToDouble(iter->first)) &&
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

std::pair<sol::object, sol::object> ipairsNext(sol::this_state lua, SharedTable table, sol::optional<unsigned long> key) {
    size_t index = key ? key.value() + 1 : 1;
    auto objKey = createStoredObject(static_cast<double>(index));
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

SharedTable SharedTable::luaSetMetatable(SharedTable& stable, const sol::stack_object& mt) {
    REQUIRE((!mt.valid()) || mt.get_type() == sol::type::table || isSharedTable(mt)) << "Unexpected type of setmetatable argument";
    std::lock_guard<SpinMutex> lock(stable.data_->lock);
    if (stable.data_->metatable != GCNull)
    {
        stable.refs_->erase(stable.data_->metatable);
        stable.data_->metatable = GCNull;
    }
    if (mt.valid()) {
        stable.data_->metatable = createStoredObject(mt)->gcHandle();
        stable.refs_->insert(stable.data_->metatable);
    }
    return stable;
}

sol::object SharedTable::luaGetMetatable(const SharedTable& stable, sol::this_state state) {
    std::lock_guard<SpinMutex> lock(stable.data_->lock);
    return stable.data_->metatable == GCNull ? sol::nil :
            sol::make_object(state, *static_cast<SharedTable*>(getGC().get(stable.data_->metatable)));
}

sol::object SharedTable::luaRawGet(const SharedTable& stable, const sol::stack_object& key, sol::this_state state) {
    return stable.rawGet(key, state);
}

SharedTable SharedTable::luaRawSet(SharedTable& stable, const sol::stack_object& key, const sol::stack_object& value) {
    stable.rawSet(key, value);
    return stable;
}

size_t SharedTable::luaSize(SharedTable& stable) {
    std::lock_guard<SpinMutex> g(stable.data_->lock);
    return stable.data_->entries.size();
}

#undef DEFFINE_METAMETHOD_CALL_0
#undef DEFFINE_METAMETHOD_CALL
#undef PROXY_METAMETHOD_IMPL

} // effil
