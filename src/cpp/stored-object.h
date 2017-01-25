#pragma once

#include <utils.h>

#include <utility>
#include <iostream>

namespace effil {

class BaseHolder {
public:
    BaseHolder() = default;
    virtual ~BaseHolder() = default;

    virtual bool compare(const BaseHolder* other) const noexcept {
        return typeid(*this) == typeid(*other);
    }

    virtual std::size_t hash() const noexcept = 0;
    virtual sol::object unpack(sol::this_state state) const = 0;

private:
    BaseHolder(const BaseHolder&) = delete;
    BaseHolder(BaseHolder&) = delete;
};

class SharedTable;

class StoredObject {
public:
    StoredObject() = default;
    StoredObject(StoredObject&& init) noexcept;
    StoredObject(SharedTable*) noexcept;
    StoredObject(const sol::object&);
    StoredObject(const sol::stack_object&);

    operator bool() const noexcept;
    std::size_t hash() const noexcept;
    sol::object unpack(sol::this_state state) const;
    StoredObject& operator=(StoredObject&& o) noexcept;
    bool operator==(const StoredObject& o) const noexcept;

private:
    std::unique_ptr<BaseHolder> data_;

private:
    StoredObject(const StoredObject&) = delete;
    StoredObject& operator=(const StoredObject&) = delete;
};

} // effil

namespace std {

// For storing as key in std::unordered_map
template<>
struct hash<effil::StoredObject> {
    std::size_t operator()(const effil::StoredObject &object) const noexcept {
        return object.hash();
    }
};

} // std
