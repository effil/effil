#pragma once

#include "garbage-collector.h"
#include "stored-object.h"
#include "spin-mutex.h"

#include <sol.hpp>

#include <unordered_map>
#include <memory>

namespace effil {

class SharedTable : public GCObject {
public:
    SharedTable();
    SharedTable(SharedTable&&) = default;
    SharedTable(const SharedTable& init);
    virtual ~SharedTable() = default;

    static sol::object getUserType(sol::state_view &lua);
    void set(StoredObject&&, StoredObject&&);

    // These functions could be invoked from lua scripts
    void luaSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue);
    sol::object luaGet(const sol::stack_object& luaKey, const sol::this_state& state) const;
    size_t size() const;

private:
    mutable std::shared_ptr<SpinMutex> lock_;
    std::shared_ptr<std::unordered_map<StoredObject, StoredObject>> data_;
};

} // effil

