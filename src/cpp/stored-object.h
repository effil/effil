#pragma once

#include "garbage-collector.h"

#include <sol.hpp>

namespace effil {

class BaseHolder {
public:
    BaseHolder() = default;
    virtual ~BaseHolder() = default;

    bool compare(const BaseHolder* other) const {
        if (typeid(*this) == typeid(*other))
            return rawCompare(other);
        return typeid(*this).before(typeid(*other));
    }

    virtual bool rawCompare(const BaseHolder* other) const = 0;
    virtual const std::type_info& type() { return typeid(*this); }
    virtual sol::object unpack(sol::this_state state) const = 0;
    virtual GCObjectHandle gcHandle() const { return GCNull; }

private:
    BaseHolder(const BaseHolder&) = delete;
    BaseHolder(BaseHolder&) = delete;
};

typedef std::unique_ptr<BaseHolder> StoredObject;

struct StoredObjectLess {
    bool operator()(const StoredObject& lhs, const StoredObject& rhs) const { return lhs->compare(rhs.get()); }
};

StoredObject createStoredObject(bool);
StoredObject createStoredObject(double);
StoredObject createStoredObject(const std::string&);
StoredObject createStoredObject(GCObjectHandle);
StoredObject createStoredObject(const sol::object&);
StoredObject createStoredObject(const sol::stack_object&);

sol::optional<bool> storedObjectToBool(const StoredObject&);
sol::optional<double> storedObjectToDouble(const StoredObject&);
sol::optional<std::string> storedObjectToString(const StoredObject&);

} // effil
