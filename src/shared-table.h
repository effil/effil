#pragma once

#include "stored-object.h"
#include "spin-mutex.h"

#include <sol.hpp>

#include <unordered_map>
#include <memory>
#include <vector>

namespace share_data {

class SharedTable {
public:
    SharedTable() = default;
    virtual ~SharedTable() = default;

    static sol::object getUserType(sol::state_view &lua) noexcept;
    void set(StoredObject, StoredObject) noexcept;
    size_t size() const noexcept;

public: // lua bindings
    void luaSet(sol::stack_object luaKey, sol::stack_object luaValue) noexcept;
    sol::object luaGet(sol::stack_object key, sol::this_state state) const noexcept;

protected:
    mutable SpinMutex lock_;
    std::unordered_map<StoredObject, StoredObject> data_;

private:
    SharedTable(const SharedTable&) = delete;
    SharedTable& operator=(const SharedTable&) = delete;
};

class TablePool {
public:
    TablePool() = default;
    SharedTable* getNew() noexcept;
    std::size_t size() const noexcept;
    void clear() noexcept;

private:
    mutable SpinMutex lock_;
    std::vector<std::unique_ptr<SharedTable>> data_;

private:
    TablePool(const TablePool&) = delete;
};

TablePool& defaultPool() noexcept;

} // share_data
