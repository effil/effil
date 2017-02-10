#include "shared-table.h"

#include "utils.h"

#include <mutex>
#include <cassert>

namespace effil {

SharedTable::SharedTable()
        : GCObject(),
          lock_(new SpinMutex()),
          data_(new std::unordered_map<StoredObject, StoredObject>()){
}

SharedTable::SharedTable(const SharedTable& init)
        : GCObject(init),
          lock_(init.lock_),
          data_(init.data_) {
}

sol::object SharedTable::getUserType(sol::state_view &lua) {
    static sol::usertype<SharedTable> type(
            "new", sol::no_constructor,
            sol::meta_function::new_index, &SharedTable::luaSet,
            sol::meta_function::index,     &SharedTable::luaGet,
            sol::meta_function::length, &SharedTable::size
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

void SharedTable::set(StoredObject&& key, StoredObject&& value) {
    std::lock_guard<SpinMutex> g(*lock_);

    if (key.isGCObject()) refs_->insert(key.gcHandle());
    if (value.isGCObject()) refs_->insert(value.gcHandle());

    (*data_)[std::move(key)] = std::move(value);
}

void SharedTable::luaSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue) {
    if (!luaKey.valid()) throw Exception() << "Indexing by nil";

    StoredObject key(luaKey);
    if (luaValue.get_type() == sol::type::nil) {
        std::lock_guard<SpinMutex> g(*lock_);

        // in this case object is not obligatory to own data
        auto it = (*data_).find(key);
        if (it != (*data_).end()) {
            if (it->first.isGCObject()) refs_->erase(it->first.gcHandle());
            if (it->second.isGCObject()) refs_->erase(it->second.gcHandle());
            (*data_).erase(it);
        }

    } else {
        set(std::move(key), StoredObject(luaValue));
    }
}

sol::object SharedTable::luaGet(const sol::stack_object& luaKey, const sol::this_state& state) const {
    if (!luaKey.valid()) throw Exception() << "Indexing by nil";

    StoredObject key(luaKey);
    std::lock_guard<SpinMutex> g(*lock_);
    auto val = (*data_).find(key);
    if (val == (*data_).end()) {
        return sol::nil;
    } else {
        return val->second.unpack(state);
    }
}

size_t SharedTable::size() const {
    std::lock_guard<SpinMutex> g(*lock_);
    return data_->size();
}

} // effil
