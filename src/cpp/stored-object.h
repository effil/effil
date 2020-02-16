#pragma once

#include "utils.h"
#include "garbage-collector.h"

#include <sol.hpp>

namespace effil {

struct EffilApiMarker{};

// Represents an interface for lua type stored at C++ code
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
    virtual GCHandle gcHandle() const { return GCNull; }
    virtual void releaseStrongReference() { }
    virtual void holdStrongReference() { }


    using DumpCache = std::unordered_map<GCHandle, int>;
    virtual sol::object convertToLua(sol::this_state state, DumpCache&) const {
        return unpack(state);
    }

private:
    BaseHolder(const BaseHolder&) = delete;
};

typedef std::shared_ptr<BaseHolder> StoredObject;

struct StoredObjectLess {
    bool operator()(const StoredObject& lhs, const StoredObject& rhs) const { return lhs->compare(rhs.get()); }
};

StoredObject createStoredObject(bool);
StoredObject createStoredObject(lua_Number);
StoredObject createStoredObject(lua_Integer);
StoredObject createStoredObject(const std::string&);
StoredObject createStoredObject(const char*);
StoredObject createStoredObject(const sol::object&);
StoredObject createStoredObject(const sol::stack_object&);

sol::optional<bool> storedObjectToBool(const StoredObject&);
sol::optional<double> storedObjectToDouble(const StoredObject&);
sol::optional<LUA_INDEX_TYPE> storedObjectToIndexType(const StoredObject&);
sol::optional<std::string> storedObjectToString(const StoredObject&);

} // effil
