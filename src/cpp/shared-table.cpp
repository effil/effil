#include "shared-table.h"

#include "utils.h"

#include <mutex>
#include <cassert>

namespace effil {

SharedTable::SharedTable() noexcept
        : GCObject(),
          data_(std::make_shared<SharedData>()) {
}

SharedTable::SharedTable(const SharedTable& init) noexcept
        : GCObject(init),
          data_(init.data_) {
}

sol::object SharedTable::getUserType(sol::state_view &lua) noexcept {
    static sol::usertype<SharedTable> type(
            "new", sol::no_constructor,
            sol::meta_function::new_index, &SharedTable::luaSet,
            sol::meta_function::index,     &SharedTable::luaGet,
            sol::meta_function::length, &SharedTable::size
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

void SharedTable::set(StoredObject&& key, StoredObject&& value) noexcept {
    std::lock_guard<SpinMutex> g(data_->lock);

    if (key->gcHandle()) refs_->insert(key->gcHandle());
    if (value->gcHandle()) refs_->insert(value->gcHandle());

    data_->entries[std::move(key)] = std::move(value);
}

void SharedTable::luaSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue) {
    ASSERT(luaKey.valid()) << "Invalid table index";

    StoredObject key = createStoredObject(luaKey);
    if (luaValue.get_type() == sol::type::nil) {
        std::lock_guard<SpinMutex> g(data_->lock);

        // in this case object is not obligatory to own data
        auto it = data_->entries.find(key);
        if (it != data_->entries.end()) {
            if (it->first->gcHandle()) refs_->erase(it->first->gcHandle());
            if (it->second->gcHandle()) refs_->erase(it->second->gcHandle());
            data_->entries.erase(it);
        }

    } else {
        set(std::move(key), createStoredObject(luaValue));
    }
}

sol::object SharedTable::luaGet(const sol::stack_object& key, const sol::this_state& state) const {
    ASSERT(key.valid());

    StoredObject cppKey = createStoredObject(key);
    std::lock_guard<SpinMutex> g(data_->lock);
    auto val = data_->entries.find(cppKey);
    if (val == data_->entries.end()) {
        return sol::nil;
    } else {
        return val->second->unpack(state);
    }
}

size_t SharedTable::size() const noexcept {
    std::lock_guard<SpinMutex> g(data_->lock);
    return data_->entries.size();
}

} // effil
