#include "shared-table.h"

#include "utils.h"

#include <mutex>
#include <cassert>

namespace effil {

SharedTable::SharedTable()
        : GCObject()
        , data_(std::make_shared<SharedData>()) {}

SharedTable::SharedTable(const SharedTable& init)
        : GCObject(init)
        , data_(init.data_) {}

sol::object SharedTable::getUserType(sol::state_view& lua) {
    sol::usertype<SharedTable> type("new", sol::no_constructor,                          //
                                           sol::meta_function::new_index, &SharedTable::luaSet, //
                                           sol::meta_function::index, &SharedTable::luaGet,     //
                                           sol::meta_function::length, &SharedTable::length,    //
                                           "__pairs", &SharedTable::pairs,                      //
                                           "__ipairs", &SharedTable::ipairs                     //
                                           );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

void SharedTable::set(StoredObject&& key, StoredObject&& value) {
    std::lock_guard<SpinMutex> g(data_->lock);

    if (key->gcHandle())
        refs_->insert(key->gcHandle());
    if (value->gcHandle())
        refs_->insert(value->gcHandle());

    data_->entries[std::move(key)] = std::move(value);
}

sol::object SharedTable::get(const StoredObject& key, const sol::this_state& state) const {
    std::lock_guard<SpinMutex> g(data_->lock);
    auto val = data_->entries.find(key);
    if (val == data_->entries.end()) {
        return sol::nil;
    } else {
        return val->second->unpack(state);
    }
}

void SharedTable::luaSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue) {
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

sol::object SharedTable::luaGet(const sol::stack_object& luaKey, const sol::this_state& state) const {
    REQUIRE(luaKey.valid()) << "Indexing by nil";
    StoredObject key = createStoredObject(luaKey);
    return get(key, state);
}

size_t SharedTable::size() const {
    std::lock_guard<SpinMutex> g(data_->lock);
    return data_->entries.size();
}

size_t SharedTable::length() const {
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
    return len;
}

SharedTable::PairsIterator SharedTable::getNext(const sol::object& key, sol::this_state lua) {
    std::lock_guard<SpinMutex> g(data_->lock);
    if (key) {
        auto obj = createStoredObject(key);
        auto upper = data_->entries.upper_bound(obj);
        if (upper != data_->entries.end())
            return std::tuple<sol::object, sol::object>(upper->first->unpack(lua), upper->second->unpack(lua));
    } else {
        if (!data_->entries.empty()) {
            const auto& begin = data_->entries.begin();
            return std::tuple<sol::object, sol::object>(begin->first->unpack(lua), begin->second->unpack(lua));
        }
    }
    return std::tuple<sol::object, sol::object>(sol::nil, sol::nil);
}

SharedTable::PairsIterator SharedTable::pairs(sol::this_state lua) const {
    auto next = [](sol::this_state lua, SharedTable table, sol::stack_object key) { return table.getNext(key, lua); };
    return std::tuple<sol::function, sol::object>(
        sol::make_object(
            lua, std::function<PairsIterator(sol::this_state lua, SharedTable table, sol::stack_object key)>(next))
            .as<sol::function>(),
        sol::make_object(lua, *this));
}

std::tuple<sol::object, sol::object> ipairsNext(sol::this_state lua, SharedTable table,
                                                sol::optional<unsigned long> key) {
    size_t index = key ? key.value() + 1 : 1;
    auto objKey = createStoredObject(static_cast<double>(index));
    sol::object value = table.get(objKey, lua);
    if (!value.valid())
        return std::tuple<sol::object, sol::object>(sol::nil, sol::nil);
    return std::tuple<sol::object, sol::object>(objKey->unpack(lua), value);
}

std::tuple<sol::function, sol::object> SharedTable::ipairs(sol::this_state lua) const {
    return std::tuple<sol::function, sol::object>(sol::make_object(lua, ipairsNext).as<sol::function>(),
                                                  sol::make_object(lua, *this));
}

} // effil
