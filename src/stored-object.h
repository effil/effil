#pragma once

#include <sol.hpp>

#include <utility>
#include <iostream>

namespace share_data {

class BaseHolder {
public:
    BaseHolder() noexcept : type_(sol::type::nil) {}
    virtual ~BaseHolder() = default;

    sol::type type() const noexcept {
        return type_;
    }

    bool compare(const BaseHolder* other) const noexcept {
        assert(other != nullptr);
        return type_ == other->type_ && rawCompare(other);
    }
    virtual bool less(const BaseHolder* other) const noexcept {
        assert(other != nullptr);
        return type_ < other->type_ && rawLess(other);
    }

    virtual bool rawCompare(const BaseHolder* other) const noexcept = 0;
    virtual bool rawLess(const BaseHolder* other) const noexcept = 0;
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
    StoredObject(sol::object) noexcept;
    StoredObject(sol::stack_object) noexcept;

    operator bool() const noexcept;
    std::size_t hash() const noexcept;
    sol::object unpack(sol::this_state state) const noexcept;
    StoredObject& operator=(StoredObject&& o) noexcept;
    bool operator==(const StoredObject& o) const noexcept;
    bool operator<(const StoredObject& o) const noexcept;

private:
    std::unique_ptr<BaseHolder> data_;

private:
    StoredObject(const StoredObject&) = delete;
    StoredObject& operator=(const StoredObject&) = delete;
};

} // share_data

namespace std {

// For storing as key in std::unordered_map
template<>
struct hash<share_data::StoredObject> {
    std::size_t operator()(const share_data::StoredObject &object) const noexcept {
        return object.hash();
    }
};

} // std
