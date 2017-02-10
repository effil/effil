#pragma once

#include "garbage-collector.h"

#include <sol.hpp>

namespace effil {

class BaseHolder {
public:
    BaseHolder() = default;
    virtual ~BaseHolder() = default;

    bool compare(const BaseHolder* other) const noexcept {
        return typeid(*this) == typeid(*other) && rawCompare(other);
    }
    virtual bool rawCompare(const BaseHolder* other) const = 0;
    virtual const std::type_info& type() { return typeid(*this); }

    virtual std::size_t hash() const noexcept = 0;
    virtual sol::object unpack(sol::this_state state) const = 0;

    virtual GCObjectHandle gcHandle() const noexcept { return GCNull; }

private:
    BaseHolder(const BaseHolder&) = delete;
    BaseHolder(BaseHolder&) = delete;
};

typedef std::unique_ptr<BaseHolder> StoredObject;

struct StoredObjectHash {
    size_t operator()(const StoredObject& o) const {
        return o->hash();
    }
};

struct StoredObjectEqual {
    bool operator()(const StoredObject& lhs, const StoredObject& rhs) const {
        return lhs->compare(rhs.get());
    }
};

StoredObject createStoredObject(bool);
StoredObject createStoredObject(double);
StoredObject createStoredObject(const std::string&);
StoredObject createStoredObject(GCObjectHandle);
StoredObject createStoredObject(const sol::object &);
StoredObject createStoredObject(const sol::stack_object &);

} // effil
