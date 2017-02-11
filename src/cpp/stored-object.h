#pragma once

#include "garbage-collector.h"

#include <sol.hpp>

namespace effil {

class BaseHolder {
public:
    BaseHolder() = default;
    virtual ~BaseHolder() = default;

    virtual bool compare(const BaseHolder* other) const { return typeid(*this) == typeid(*other); }
    virtual const std::type_info& type() { return typeid(*this); }

    virtual std::size_t hash() const = 0;
    virtual sol::object unpack(sol::this_state state) const = 0;

private:
    BaseHolder(const BaseHolder&) = delete;
    BaseHolder(BaseHolder&) = delete;
};

class StoredObject {
public:
    StoredObject() = default;
    StoredObject(StoredObject&& init);
    StoredObject(GCObjectHandle);
    StoredObject(const sol::object&);
    StoredObject(const sol::stack_object&);


    operator bool() const;
    std::size_t hash() const;
    sol::object unpack(sol::this_state state) const;

    bool isGCObject() const;
    GCObjectHandle gcHandle() const;

    StoredObject& operator=(StoredObject&& o);
    bool operator==(const StoredObject& o) const;

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
