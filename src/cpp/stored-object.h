#pragma once

#include "utils.h"
#include "garbage-collector.h"

#include <sol.hpp>

namespace effil {

struct EffilApiMarker{};

class SharedTable;
class Function;
class Channel;
class Thread;

// Represents an interface for lua type stored at C++ code
class StoredObject {
public:
    StoredObject(StoredObject&& other);
    StoredObject();
    ~StoredObject();

    explicit StoredObject(const std::string& str);
    explicit StoredObject(lua_Number num);
    explicit StoredObject(lua_Integer num);
    explicit StoredObject(bool b);
    explicit StoredObject(const SharedTable& obj);
    explicit StoredObject(const Function& obj);
    explicit StoredObject(const Channel& obj);
    explicit StoredObject(const Thread& obj);
    explicit StoredObject(sol::lightuserdata_value ud);
    explicit StoredObject(sol::nil_t);
    explicit StoredObject(EffilApiMarker);

    bool operator==(const StoredObject& other) const;
    const std::type_info& type();
    sol::object unpack(sol::this_state state) const;
    GCHandle gcHandle() const;
    void releaseStrongReference();
    void holdStrongReference();

    sol::optional<LUA_INDEX_TYPE> toIndexType() const;

protected:
    union {
        lua_Number  number_;
        lua_Integer integer_;
        bool        bool_;
        char*       string_;
        GCHandle    handle_;
        void*       lightUData_;
    };
    StrongRef strongRef_;

    enum class StoredType {
        Number,
        Integer,
        Boolean,
        String,
        Nil,
        ApiMarker,
        LightUserData,
        SharedTable,
        SharedChannel,
        SharedFunction,
        SharedThread,
    } type_;

    StoredObject(const StoredObject&) = delete;
    friend struct StoredObjectHash;
};

struct StoredObjectHash {
    size_t operator ()(const StoredObject& obj) const;
};

StoredObject createStoredObject(bool);
StoredObject createStoredObject(lua_Number);
StoredObject createStoredObject(lua_Integer);
StoredObject createStoredObject(const std::string&);
StoredObject createStoredObject(const char*);
StoredObject createStoredObject(const sol::object&);
StoredObject createStoredObject(const sol::stack_object&);

} // effil
