#include "shared-table.h"

#include <cassert>
#include <mutex>

namespace effil {

sol::object SharedTable::getUserType(sol::state_view &lua) noexcept {
    static sol::usertype<SharedTable> type(
            sol::call_construction(), sol::default_constructor,
            sol::meta_function::new_index, &SharedTable::luaSet,
            sol::meta_function::index,     &SharedTable::luaGet,
            sol::meta_function::length, &SharedTable::size
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

void SharedTable::set(StoredObject key, StoredObject value) noexcept {
    std::lock_guard<SpinMutex> g(lock_);
    data_[std::move(key)] = std::move(value);
}

void SharedTable::luaSet(sol::stack_object luaKey, sol::stack_object luaValue) noexcept {
    if (!luaKey.valid())
        throw sol::error("Invalid table index");

    StoredObject key(luaKey);
    if (luaValue.get_type() == sol::type::nil) {
        std::lock_guard<SpinMutex> g(lock_);
        // in this case object is not obligatory to own data
        data_.erase(key);
    } else {
        StoredObject value(luaValue);
        std::lock_guard<SpinMutex> g(lock_);
        data_[std::move(key)] = std::move(value);
    }
}

sol::object SharedTable::luaGet(sol::stack_object key, sol::this_state state) const noexcept {
    assert(key.valid());

    StoredObject cppKey(key);
    std::lock_guard<SpinMutex> g(lock_);
    auto val = data_.find(cppKey);
    if (val == data_.end()) {
        return sol::nil;
    } else {
        return val->second.unpack(state);
    }
}

size_t SharedTable::size() const noexcept {
    std::lock_guard<SpinMutex> g(lock_);
    return data_.size();
}

SharedTable* TablePool::getNew() noexcept {
    SharedTable* ptr = new SharedTable();
    std::lock_guard<SpinMutex> g(lock_);
    data_.emplace_back(ptr);
    return ptr;
}
std::size_t TablePool::size() const noexcept {
    std::lock_guard<SpinMutex> g(lock_);
    return data_.size();
}

void TablePool::clear() noexcept {
    std::lock_guard<SpinMutex> g(lock_);
    data_.clear();
}

TablePool& defaultPool() noexcept {
    static TablePool pool;
    return pool;
}

} // effil
