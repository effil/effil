#include "shared-table.h"

#include <cassert>
#include <mutex>

namespace share_data {

sol::object SharedTable::get_user_type(sol::state_view& lua) noexcept {
    static sol::usertype<share_data::SharedTable> type(
            sol::call_construction(), sol::default_constructor,
            sol::meta_function::new_index, &share_data::SharedTable::luaSet,
            sol::meta_function::index,     &share_data::SharedTable::luaGet,
            sol::meta_function::length, &SharedTable::size
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
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
        data_[std::move(key)] = std::move(value);
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

} // share_data
