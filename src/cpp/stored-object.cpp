#include "stored-object.h"
#include "channel.h"
#include "threading.h"
#include "shared-table.h"
#include "function.h"
#include "utils.h"
#include "thread_runner.h"

#include <map>
#include <vector>
#include <algorithm>

#include <cassert>

namespace effil {

namespace {

class ApiReferenceHolder : public BaseHolder {
public:
    bool rawCompare(const BaseHolder*) const noexcept final { return true; }
    sol::object unpack(sol::this_state lua) const final {
        luaopen_effil(lua);
        return sol::stack::pop<sol::object>(lua);
    }
};

class NilHolder : public BaseHolder {
public:
    bool rawCompare(const BaseHolder*) const noexcept final { return true; }
    sol::object unpack(sol::this_state) const final { return sol::nil; }
};

template <typename StoredType>
class PrimitiveHolder : public BaseHolder {
public:
    PrimitiveHolder(const sol::stack_object& luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {}

    PrimitiveHolder(const sol::object& luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {}

    PrimitiveHolder(const StoredType& init) noexcept
            : data_(init) {}

    bool rawCompare(const BaseHolder* other) const noexcept final {
        return data_ < static_cast<const PrimitiveHolder<StoredType>*>(other)->data_;
    }

    sol::object unpack(sol::this_state state) const final { return sol::make_object(state, data_); }

    StoredType getData() { return data_; }

private:
    StoredType data_;
};

template<typename T>
class GCObjectHolder : public BaseHolder {
public:
    template <typename SolType>
    GCObjectHolder(const SolType& luaObject) {
        assert(luaObject.template is<T>());
        strongRef_ = luaObject.template as<T>();
        handle_ = strongRef_->handle();
    }

    GCObjectHolder(GCHandle handle)
            : handle_(handle) {
        strongRef_ = GC::instance().get<T>(handle_);
    }

    bool rawCompare(const BaseHolder* other) const final {
        return handle_ < static_cast<const GCObjectHolder<T>*>(other)->handle_;
    }

    sol::object unpack(sol::this_state state) const override {
        return sol::make_object(state, GC::instance().get<T>(handle_));
    }

    GCHandle gcHandle() const override { return handle_; }

    void releaseStrongReference() override {
        strongRef_ = sol::nullopt;
    }

    void holdStrongReference() override {
        if (!strongRef_) {
            strongRef_ = GC::instance().get<T>(handle_);
        }
    }

protected:
    GCHandle handle_;
    sol::optional<T> strongRef_;
};

class SharedTableHolder : public GCObjectHolder<SharedTable> {
public:
    using GCObjectHolder<SharedTable>::GCObjectHolder;

    sol::object convertToLua(sol::this_state state, DumpCache& cache) const final {
        return GC::instance().get<SharedTable>(handle_).luaDump(state, cache);
    }
};

class FunctionHolder : public GCObjectHolder<Function> {
public:
    template <typename SolType>
    FunctionHolder(const SolType& luaObject) : GCObjectHolder<Function>(luaObject) {}

    sol::object unpack(sol::this_state state) const final {
        return GC::instance().get<Function>(handle_).loadFunction(state);
    }

    sol::object convertToLua(sol::this_state state, DumpCache& cache) const final {
        return GC::instance().get<Function>(handle_).convertToLua(state, cache);
    }
};

class CFunctionHolder : public BaseHolder {
public:
    CFunctionHolder(sol::state_view state, int stack_index)
        : BaseHolder()
    {
        cfunction_ = lua_tocfunction(state, stack_index);
        REQUIRE(cfunction_ != nullptr) << "can't get C function pointer";
    }

    sol::object unpack(sol::this_state state) const final {
        lua_pushcfunction(state, cfunction_);
        return sol::stack::pop<sol::object>(state);
    }

    bool rawCompare(const BaseHolder* other) const override {
        const auto cfh = dynamic_cast<const CFunctionHolder*>(other);
        return cfh && cfh->cfunction_ == cfunction_;
    }

private:
    lua_CFunction cfunction_;
};

void dumpTable(SharedTable& target, const sol::table& luaTable, SolTableToShared& visited);

StoredObject makeStoredObject(const sol::object& luaObject, SolTableToShared& visited) {
    if (luaObject.get_type() == sol::type::table) {
        sol::table luaTable = luaObject;
        auto st = std::find_if(visited.begin(), visited.end(), [&](const auto& pair) {
            return pair.first == luaObject;
        });
        if (st == visited.end()) {
            SharedTable table = GC::instance().create<SharedTable>();
            visited.push_back({luaTable, table.handle()});
            dumpTable(table, luaTable, visited);
            return std::make_unique<SharedTableHolder>(table.handle());
        } else {
            return std::make_unique<SharedTableHolder>(st->second);
        }
    } else {
        return createStoredObject(luaObject, visited);
    }
}

void dumpTable(SharedTable& target, const sol::table& luaTable, SolTableToShared& visited) {
    for (auto& row : luaTable) {
        target.set(makeStoredObject(row.first, visited), makeStoredObject(row.second, visited));
    }
}

typedef std::shared_ptr<SpinMutex> MutexPtr;

template <typename SolObject>
StoredObject fromSolObject(const SolObject& luaObject, SolTableToShared& visited) {
    switch (luaObject.get_type()) {
        case sol::type::nil:
            return std::make_unique<NilHolder>();
            break;
        case sol::type::boolean:
            return std::make_unique<PrimitiveHolder<bool>>(luaObject);
        case sol::type::number:
        {
#if LUA_VERSION_NUM == 503
            sol::stack::push(luaObject.lua_state(), luaObject);
            int isInterger = lua_isinteger(luaObject.lua_state(), -1);
            sol::stack::pop<sol::object>(luaObject.lua_state());
            if (isInterger)
                return std::make_unique<PrimitiveHolder<lua_Integer>>(luaObject);
            else
#endif // Lua5.3
                return std::make_unique<PrimitiveHolder<lua_Number>>(luaObject);
        }
        case sol::type::string:
            return std::make_unique<PrimitiveHolder<std::string>>(luaObject);
        case sol::type::lightuserdata:
            return std::make_unique<PrimitiveHolder<void*>>(luaObject);
        case sol::type::userdata:
            if (luaObject.template is<SharedTable>())
                return std::make_unique<SharedTableHolder>(luaObject);
            else if (luaObject.template is<Channel>())
                return std::make_unique<GCObjectHolder<Channel>>(luaObject);
            else if (luaObject.template is<Function>())
                return std::make_unique<FunctionHolder>(luaObject);
            else if (luaObject.template is<Thread>())
                return std::make_unique<GCObjectHolder<Thread>>(luaObject);
            else if (luaObject.template is<EffilApiMarker>())
                return std::make_unique<ApiReferenceHolder>();
            else if (luaObject.template is<ThreadRunner>())
                return std::make_unique<GCObjectHolder<ThreadRunner>>(luaObject);
            else if (luaObject.template is<MutexPtr>())
                return std::make_unique<PrimitiveHolder<MutexPtr>>(luaObject);
            else
                throw Exception() << "Unable to store userdata object";
        case sol::type::function: {
            {
                auto poper = sol::stack::push_pop(luaObject);
                if (lua_iscfunction(luaObject.lua_state(), -1))
                    return std::make_unique<CFunctionHolder>(luaObject.lua_state(), -1);
            }
            Function func = GC::instance().create<Function>(luaObject, visited);
            return std::make_unique<FunctionHolder>(func.handle());
        }
        case sol::type::table: {
            sol::table luaTable = luaObject;

            const auto iter = std::find_if(visited.begin(), visited.end(), [&](const auto& pair) {
                return pair.first == luaTable;
            });
            if (iter != visited.end()) {
                return std::make_unique<SharedTableHolder>(iter->second);
            }
            // Tables pool is used to store tables.
            // Right now not defiantly clear how ownership between states works.
            SharedTable table = GC::instance().create<SharedTable>();
            visited.push_back({luaTable, table.handle()});

            // Let's dump table and all subtables
            // SolTableToShared is used to prevent from infinity recursion
            // in recursive tables
            dumpTable(table, luaTable, visited);

            const sol::table luaMetatable = luaTable[sol::metatable_key];
            if (luaMetatable.valid()) {
                SharedTable metaTable = GC::instance().create<SharedTable>();
                dumpTable(metaTable, luaMetatable, visited);
                table.setMetatable(metaTable);
            }
            return std::make_unique<SharedTableHolder>(table.handle());
        }
        default:
            throw Exception() << "unable to store object of " << luaTypename(luaObject) << " type";
    }
    return nullptr;
}

} // namespace

StoredObject createStoredObject(bool value) { return std::make_unique<PrimitiveHolder<bool>>(value); }

StoredObject createStoredObject(lua_Number value) { return std::make_unique<PrimitiveHolder<lua_Number>>(value); }

StoredObject createStoredObject(lua_Integer value) { return std::make_unique<PrimitiveHolder<lua_Integer>>(value); }

StoredObject createStoredObject(const std::string& value) {
    return std::make_unique<PrimitiveHolder<std::string>>(value);
}

StoredObject createStoredObject(const char* value) {
    return std::make_unique<PrimitiveHolder<std::string>>(value);
}

StoredObject createStoredObject(const sol::object& object) {
    SolTableToShared visited;
    return fromSolObject(object, visited);
}

StoredObject createStoredObject(const sol::stack_object& object) {
    SolTableToShared visited;
    return fromSolObject(object, visited);
}

StoredObject createStoredObject(const sol::object& obj, SolTableToShared& visited) {
    return fromSolObject(obj, visited);
}

StoredObject createStoredObject(const sol::stack_object& obj, SolTableToShared& visited) {
    return fromSolObject(obj, visited);
}

template <typename DataType>
sol::optional<DataType> getPrimitiveHolderData(const StoredObject& sobj) {
    auto ptr = dynamic_cast<PrimitiveHolder<DataType>*>(sobj.get());
    if (ptr)
        return ptr->getData();
    return sol::optional<DataType>();
}

sol::optional<bool> storedObjectToBool(const StoredObject& sobj) { return getPrimitiveHolderData<bool>(sobj); }

sol::optional<double> storedObjectToDouble(const StoredObject& sobj) { return getPrimitiveHolderData<double>(sobj); }

sol::optional<LUA_INDEX_TYPE> storedObjectToIndexType(const StoredObject& sobj) { return getPrimitiveHolderData<LUA_INDEX_TYPE>(sobj); }

sol::optional<std::string> storedObjectToString(const StoredObject& sobj) {
    return getPrimitiveHolderData<std::string>(sobj);
}

template<>
sol::optional<SharedTable> storedObjectTo(const StoredObject& obj) {
    if (const auto ptr = std::dynamic_pointer_cast<SharedTableHolder>(obj)) {
        return GC::instance().get<SharedTable>(ptr->gcHandle());
    }
    return sol::nullopt;
}

template<>
sol::optional<Function> storedObjectTo(const StoredObject& obj) {
    if (const auto ptr = std::dynamic_pointer_cast<FunctionHolder>(obj)) {
        return GC::instance().get<Function>(ptr->gcHandle());
    }
    return sol::nullopt;
}

} // effil
