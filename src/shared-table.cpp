#include "shared-table.h"

#include <cassert>
#include <mutex>

namespace core {

void SharedTable::bind(sol::state_view& lua) noexcept {
    lua.new_usertype<SharedTable>("shared_table",
                                  sol::meta_function::new_index, &SharedTable::luaSet,
                                  sol::meta_function::index, &SharedTable::luaGet,
                                  sol::meta_function::length, &SharedTable::size);
}

void SharedTable::luaSet(sol::stack_object luaKey, sol::stack_object luaValue) noexcept {
    assert(luaKey.valid());

    StoredObject key(luaKey);
    if (luaValue.get_type() == sol::type::nil) {
        std::lock_guard<SpinMutex> g(lock_);
        // in this case object is not obligatory to own data
        data_.erase(key);
    } else {
        StoredObject value(luaValue);
        std::lock_guard<SpinMutex> g(lock_);
        data_.emplace(std::make_pair(std::move(key), std::move(value)));
    }
}

sol::object SharedTable::luaGet(sol::stack_object key, sol::this_state state) noexcept {
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

size_t SharedTable::size() noexcept {
    std::lock_guard<SpinMutex> g(lock_);
    return data_.size();
}

SharedTable* TablePool::getNew() noexcept {
    std::lock_guard<SpinMutex> g(lock_);
    data_.push_back(std::make_unique<SharedTable>());
    return (*data_.rend()).get();
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

} // core