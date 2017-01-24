#pragma once

#include <utils.h>

#include <utility>
#include <iostream>

namespace effil {

class BaseHolder {
public:
    BaseHolder(sol::type t) noexcept : type_(t) {}
    virtual ~BaseHolder() = default;

    sol::type type() const noexcept {
        return type_;
    }

    bool compare(const BaseHolder* other) const noexcept {
        ASSERT(other != nullptr);
        return type_ == other->type_ && rawCompare(other);
    }

    virtual bool rawCompare(const BaseHolder* other) const noexcept = 0;
    virtual std::size_t hash() const noexcept = 0;
    virtual sol::object unpack(sol::this_state state) const noexcept = 0;

protected:
    sol::type type_;

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
    StoredObject(const sol::object&) noexcept;
    StoredObject(const sol::stack_object&) noexcept;

    operator bool() const noexcept;
    std::size_t hash() const noexcept;
    sol::object unpack(sol::this_state state) const noexcept;
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
