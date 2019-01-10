#include "stored-object.h"
#include "channel.h"
#include "threading.h"
#include "shared-table.h"
#include "function.h"

#include <map>
#include <vector>
#include <algorithm>

#include <cassert>

namespace effil {

StoredObject::StoredObject(StoredObject&& other) {
    type_ = other.type_;
    switch (type_) {
        case StoredType::Number:
            number_ = other.number_;
            break;
        case StoredType::Integer:
            integer_ = other.integer_;
            break;
        case StoredType::Boolean:
            bool_ = other.bool_;
            break;
        case StoredType::String:
            string_ = other.string_;
            other.string_ = nullptr;
            break;
        case StoredType::LightUserData:
            lightUData_ = other.lightUData_;
            other.lightUData_ = nullptr;
            break;
        case StoredType::SharedTable:
        case StoredType::SharedChannel:
        case StoredType::SharedFunction:
        case StoredType::SharedThread:
            handle_ = other.handle_;
            strongRef_ = other.strongRef_;
            other.handle_ = nullptr;
            other.strongRef_.reset();
            break;
        default:
            break;
    }
}

StoredObject::StoredObject(const SharedTable& obj)
    : handle_(obj.handle()), type_(StoredType::SharedTable) {
    holdStrongReference();
}

StoredObject::StoredObject(const Function& obj)
    : handle_(obj.handle()), type_(StoredType::SharedFunction) {
    holdStrongReference();
}

StoredObject::StoredObject(const Channel& obj)
    : handle_(obj.handle()), type_(StoredType::SharedChannel) {
    holdStrongReference();
}

StoredObject::StoredObject(const Thread& obj)
    : handle_(obj.handle()), type_(StoredType::SharedThread) {
    holdStrongReference();
}

StoredObject::StoredObject(const std::string& str) : type_(StoredType::String) {
    string_ = (char*)malloc(str.size() + 1);
    strcpy(string_, str.c_str());
}

StoredObject::StoredObject(lua_Number num)
    : number_(num), type_(StoredType::Number) {}

StoredObject::StoredObject(lua_Integer num)
    : integer_(num), type_(StoredType::Integer) {}

StoredObject::StoredObject(bool b) : bool_(b), type_(StoredType::Boolean) {}

StoredObject::StoredObject(sol::lightuserdata_value ud)
    : lightUData_(ud.value), type_(StoredType::LightUserData) {}

StoredObject::StoredObject(sol::nil_t) : bool_(false), type_(StoredType::Nil) {}

StoredObject::StoredObject(EffilApiMarker) : type_(StoredType::ApiMarker) {}

StoredObject::StoredObject() : type_(StoredType::Nil) {}

StoredObject::~StoredObject() {
    if (type_ == StoredType::String)
        free(string_);
}

bool StoredObject::operator==(const StoredObject& other) const {
    if (type_ == other.type_) {
        switch (type_) {
            case StoredType::Number:
                return number_ == other.number_;
            case StoredType::Integer:
                return integer_ == other.integer_;
            case StoredType::Boolean:
                return bool_ == other.bool_;
            case StoredType::String:
                return strcmp(other.string_, string_) == 0;
            case StoredType::LightUserData:
                return lightUData_ == other.lightUData_;
            case StoredType::SharedTable:
            case StoredType::SharedChannel:
            case StoredType::SharedFunction:
            case StoredType::SharedThread:
                return handle_ == other.handle_;
            default:
                return false;
        }
    }
    return false;
}

const std::type_info& StoredObject::type() {
    return typeid(*this);
}

sol::object StoredObject::unpack(sol::this_state state) const {
    switch (type_) {
        case StoredType::Number:
            return sol::make_object(state, number_);
        case StoredType::Integer:
            return sol::make_object(state, integer_);
        case StoredType::Boolean:
            return sol::make_object(state, bool_);
        case StoredType::String:
            return sol::make_object(state, std::string(string_));
        case StoredType::LightUserData:
            return sol::make_object(state, lightUData_);
        case StoredType::ApiMarker:
        {
            luaopen_effil(state);
            return sol::stack::pop<sol::object>(state);
        }
        case StoredType::SharedTable:
            return sol::make_object(state, GC::instance().get<SharedTable>(handle_));
        case StoredType::SharedChannel:
            return sol::make_object(state, GC::instance().get<Channel>(handle_));
        case StoredType::SharedThread:
            return sol::make_object(state, GC::instance().get<Thread>(handle_));
        case StoredType::SharedFunction:
            return sol::make_object(state, GC::instance().get<Function>(handle_)
                                    .loadFunction(state));
        case StoredType::Nil:
        default:
            return sol::nil;
    }
}

GCHandle StoredObject::gcHandle() const {
    if (type_ == StoredType::SharedThread ||
            type_ == StoredType::SharedChannel ||
            type_ == StoredType::SharedTable ||
            type_ == StoredType::SharedFunction) {
        return handle_;
    }
    else {
        return GCNull;
    }
}

void StoredObject::releaseStrongReference() {
    strongRef_.reset();
}

void StoredObject::holdStrongReference() {
    if (type_ == StoredType::SharedThread ||
            type_ == StoredType::SharedChannel ||
            type_ == StoredType::SharedTable ||
            type_ == StoredType::SharedFunction) {
        strongRef_ = GC::instance().getRef(handle_);
    }
}

template<typename T>
inline size_t bindHashes(size_t seed, const T& obj) {
    std::hash<T> hasher;
    return seed ^ (hasher(obj) + 0x9e3779b9 + (seed<<6) + (seed>>2));
}

size_t StoredObjectHash::operator ()(const StoredObject& obj) const {
    const auto seed = std::hash<size_t>()((size_t)obj.type_);

    typedef StoredObject::StoredType Type;

    switch (obj.type_) {
        case Type::Number:
            return bindHashes(seed, obj.number_);
        case Type::Integer:
            return bindHashes(seed, obj.integer_);
        case Type::Boolean:
            return bindHashes(seed, obj.bool_);
        case Type::String:
            return bindHashes<std::string>(seed, obj.string_);
        case Type::LightUserData:
            return bindHashes(seed, obj.lightUData_);
        case Type::SharedTable:
        case Type::SharedChannel:
        case Type::SharedFunction:
        case Type::SharedThread:
            return bindHashes(seed, obj.handle_);
        default:
            assert(false);
            return 0;
    }
    return seed;
}

sol::optional<LUA_INDEX_TYPE> StoredObject::toIndexType() const {
    if (type_ == StoredType::Number)
        return number_;
    else
        return sol::nullopt;
}

namespace {

// This class is used as a storage for visited sol::tables
// TODO: try to use map or unordered map instead of linear search in vector
// TODO: Trick is - sol::object has only operator==:/
typedef std::unordered_map<int, SharedTable> SolTableToShared;

void dumpTable(SharedTable& target, const sol::table& luaTable, SolTableToShared& visited);

StoredObject makeStoredObject(const sol::object& luaObject, SolTableToShared& visited) {
    if (luaObject.get_type() == sol::type::table) {
        const sol::table luaTable = luaObject;
        auto st = visited.find(luaTable.registry_index());;
        if (st == visited.end()) {
            SharedTable table = GC::instance().create<SharedTable>();
            visited.emplace(luaTable.registry_index(), table);
            dumpTable(table, luaTable, visited);
            return StoredObject(table);
        } else {
            return StoredObject(st->second);
        }
    } else {
        return createStoredObject(luaObject);
    }
}

void dumpTable(SharedTable& target, const sol::table& luaTable, SolTableToShared& visited) {
    target.reserve(luaTable.size());
    for (const auto& row : luaTable) {
        target.set(makeStoredObject(row.first, visited), makeStoredObject(row.second, visited));
    }
}

template <typename SolObject>
StoredObject fromSolObject(const SolObject& luaObject) {
    switch (luaObject.get_type()) {
        case sol::type::nil:
            return StoredObject(sol::nil);
        case sol::type::boolean:
            return StoredObject(luaObject.template as<bool>());
        case sol::type::number:
        {
#if LUA_VERSION_NUM == 503
            sol::stack::push(luaObject.lua_state(), luaObject);
            int isInterger = lua_isinteger(luaObject.lua_state(), -1);
            sol::stack::pop<sol::object>(luaObject.lua_state());
            if (isInterger)
                return StoredObject(luaObject.template as<lua_Integer>());
            else
#endif // Lua5.3
                return StoredObject(luaObject.template as<lua_Number>());
        }
        case sol::type::string:
            return StoredObject(luaObject.template as<std::string>());
        case sol::type::lightuserdata:
            return StoredObject(sol::lightuserdata_value(luaObject.template as<void*>()));
        case sol::type::userdata:
            if (luaObject.template is<SharedTable>())
                return StoredObject(luaObject.template as<SharedTable>());
            else if (luaObject.template is<Channel>())
                return StoredObject(luaObject.template as<Channel>());
            else if (luaObject.template is<Function>())
                return StoredObject(luaObject.template as<Function>());
            else if (luaObject.template is<Thread>())
                return StoredObject(luaObject.template as<Thread>());
            else if (luaObject.template is<EffilApiMarker>())
                return StoredObject(EffilApiMarker());
            else
                throw Exception() << "Unable to store userdata object";
        case sol::type::function: {
            Function func = GC::instance().create<Function>(luaObject);
            return StoredObject(func);
        }
        case sol::type::table: {
            sol::table luaTable = luaObject;
            // Tables pool is used to store tables.
            // Right now not defiantly clear how ownership between states works.
            SharedTable table = GC::instance().create<SharedTable>();
            SolTableToShared visited{{luaTable.registry_index(), table}};

            // Let's dump table and all subtables
            // SolTableToShared is used to prevent from infinity recursion
            // in recursive tables
            dumpTable(table, luaTable, visited);
            return StoredObject(table);
        }
        default:
            throw Exception() << "unable to store object of " << luaTypename(luaObject) << " type";
    }
}

} // namespace

StoredObject createStoredObject(bool value) { return StoredObject(value); }

StoredObject createStoredObject(lua_Number value) { return StoredObject(value); }

StoredObject createStoredObject(lua_Integer value) { return StoredObject(value); }

StoredObject createStoredObject(const std::string& value) {
    return StoredObject(value);
}

StoredObject createStoredObject(const char* value) {
    return StoredObject(std::string(value));
}

StoredObject createStoredObject(const sol::object& object) { return fromSolObject(object); }

StoredObject createStoredObject(const sol::stack_object& object) { return fromSolObject(object); }

} // effil
