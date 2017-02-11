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
    SharedTable() noexcept;
    SharedTable(SharedTable&&) = default;
    SharedTable(const SharedTable& init) noexcept;
    virtual ~SharedTable() = default;

    static sol::object getUserType(sol::state_view &lua) noexcept;
    void set(StoredObject&&, StoredObject&&) noexcept;

    // These functions could be invoked from lua scripts
    void luaSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue);
    sol::object luaGet(const sol::stack_object& key, const sol::this_state& state) const;
    size_t size() const noexcept;

private:
    typedef std::unique_ptr<BaseHolder> StoredObject;
    struct SharedData {
        SpinMutex lock;
        std::unordered_map<StoredObject, StoredObject, StoredObjectHash, StoredObjectEqual> entries;
    };

private:
    std::shared_ptr<SharedData> data_;
};

} // effil

