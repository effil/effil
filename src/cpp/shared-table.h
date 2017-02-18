#pragma once

#include "garbage-collector.h"
#include "stored-object.h"
#include "spin-mutex.h"

#include <sol.hpp>

#include <unordered_map>
#include <memory>

namespace effil {

class SharedTable : public GCObject {
private:
    typedef std::tuple<sol::function, sol::object> PairsIterator;
    typedef std::map<StoredObject, StoredObject, StoredObjectLess> DataEntries;

public:
    SharedTable();
    SharedTable(SharedTable&&) = default;
    SharedTable(const SharedTable& init);
    virtual ~SharedTable() = default;

    static sol::object getUserType(sol::state_view &lua);
    void set(StoredObject&&, StoredObject&&);
    sol::object get(const StoredObject& key, const sol::this_state& state) const;
    PairsIterator getNext(const sol::object& key, sol::this_state lua);

    // These functions could be invoked from lua scripts
    void luaSet(const sol::stack_object& luaKey, const sol::stack_object& luaValue);
    sol::object luaGet(const sol::stack_object& key, const sol::this_state& state) const;
    size_t size() const;
    size_t length() const;
    PairsIterator pairs(sol::this_state) const;
    PairsIterator ipairs(sol::this_state) const;

private:

    struct SharedData {
        SpinMutex lock;
        DataEntries entries;
    };

    std::shared_ptr<SharedData> data_;
};

} // effil

