#include "stored-object.h"

#include "shared-table.h"
#include "utils.h"

#include <map>
#include <vector>
#include <algorithm>

#include <cassert>

namespace effil {

namespace {

template<typename StoredType>
class PrimitiveHolder : public BaseHolder {
public:
    PrimitiveHolder(const sol::stack_object& luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {}

    PrimitiveHolder(const sol::object& luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {}

    PrimitiveHolder(const StoredType& init) noexcept
            : data_(init) {}

    bool rawCompare(const BaseHolder* other) const noexcept final {
        return static_cast<const PrimitiveHolder<StoredType>*>(other)->data_ == data_;
    }

    std::size_t hash() const noexcept final {
        return std::hash<StoredType>()(data_);
    }

    sol::object unpack(sol::this_state state) const final {
        return sol::make_object(state, data_);
    }

private:
    StoredType data_;
};

class FunctionHolder : public BaseHolder {
public:
    template<typename SolObject>
    FunctionHolder(SolObject luaObject) noexcept {
        sol::state_view lua(luaObject.lua_state());
        sol::function dumper = lua["string"]["dump"];
        ASSERT(dumper.valid());
        function_ = dumper(luaObject);
    }

    bool rawCompare(const BaseHolder* other) const noexcept final {
        return static_cast<const FunctionHolder*>(other)->function_ == function_;
    }

    std::size_t hash() const noexcept final {
        return std::hash<std::string>()(function_);
    }

    sol::object unpack(sol::this_state state) const final {
        sol::state_view lua((lua_State*)state);
        sol::function loader = lua["loadstring"];
        ASSERT(loader.valid());
        sol::function result = loader(function_);
        ASSERT(result.valid()) << "Unable to restore function!\n" << "Content:\n" << function_;
        return sol::make_object(state, result);
    }

private:
    std::string function_;
};

class TableHolder : public BaseHolder {
public:
    template<typename SolType>
    TableHolder(const SolType& luaObject) noexcept {
        assert(luaObject.template is<SharedTable>());
        handle_ = luaObject.template as<SharedTable>().handle();
        assert(getGC().has(handle_));
    }

    TableHolder(GCObjectHandle handle) noexcept
            : handle_(handle) {}

    bool rawCompare(const BaseHolder *other) const noexcept final {
        return static_cast<const TableHolder*>(other)->handle_ == handle_;
    }

    std::size_t hash() const noexcept final {
        return std::hash<GCObjectHandle>()(handle_);
    }

    sol::object unpack(sol::this_state state) const noexcept final {
        return sol::make_object(state, *static_cast<SharedTable*>(getGC().get(handle_)));
    }

    GCObjectHandle gcHandle() const noexcept override { return  handle_; }
private:
    GCObjectHandle handle_;
};

// This class is used as a storage for visited sol::tables
// TODO: try to use map or unordered map instead of linear search in vector
// TODO: Trick is - sol::object has only operator==:/
typedef std::vector<std::pair<sol::object, GCObjectHandle>> SolTableToShared;

void dumpTable(SharedTable* target, sol::table luaTable, SolTableToShared& visited);

StoredObject makeStoredObject(sol::object luaObject, SolTableToShared& visited) {
    if (luaObject.get_type() == sol::type::table) {
        sol::table luaTable = luaObject;
        auto comparator = [&luaTable](const std::pair<sol::table, GCObjectHandle>& element){
            return element.first == luaTable;
        };
        auto st = std::find_if(visited.begin(), visited.end(), comparator);

        if (st == std::end(visited)) {
            SharedTable table = getGC().create<SharedTable>();
            visited.emplace_back(std::make_pair(luaTable, table.handle()));
            dumpTable(&table, luaTable, visited);
            return createStoredObject(table.handle());
        } else {
            return createStoredObject(st->second);
        }
    } else {
        return createStoredObject(luaObject);
    }
}

void dumpTable(SharedTable* target, sol::table luaTable, SolTableToShared& visited) {
    for(auto& row : luaTable) {
        target->set(makeStoredObject(row.first, visited), makeStoredObject(row.second, visited));
    }
}

template<typename SolObject>
StoredObject fromSolObject(const SolObject& luaObject) {
    switch(luaObject.get_type()) {
        case sol::type::nil:
            break;
        case sol::type::boolean:
            return std::make_unique<PrimitiveHolder<bool>>(luaObject);
        case sol::type::number:
            return std::make_unique<PrimitiveHolder<double>>(luaObject);
        case sol::type::string:
            return std::make_unique<PrimitiveHolder<std::string>>(luaObject);
        case sol::type::userdata:
            return std::make_unique<TableHolder>(luaObject);
        case sol::type::function:
            return std::make_unique<FunctionHolder>(luaObject);
        case sol::type::table:
        {
            sol::table luaTable = luaObject;
            // Tables pool is used to store tables.
            // Right now not defiantly clear how ownership between states works.
            SharedTable table = getGC().create<SharedTable>();
            SolTableToShared visited{{luaTable, table.handle()}};

            // Let's dump table and all subtables
            // SolTableToShared is used to prevent from infinity recursion
            // in recursive tables
            dumpTable(&table, luaTable, visited);
            return std::make_unique<TableHolder>(table.handle());
        }
        default:
            ERROR << "Unable to store object of that type: " << (int)luaObject.get_type() << "\n";
    }
    return nullptr;
}

} // namespace

StoredObject createStoredObject(bool value) {
    return std::make_unique<PrimitiveHolder<bool>>(value);
}

StoredObject createStoredObject(double value) {
    return std::make_unique<PrimitiveHolder<double>>(value);
}

StoredObject createStoredObject(const std::string& value) {
    return std::make_unique<PrimitiveHolder<std::string>>(value);
}

StoredObject createStoredObject(const sol::object &object) {
    return fromSolObject(object);
}

StoredObject createStoredObject(const sol::stack_object &object) {
    return fromSolObject(object);
}

StoredObject createStoredObject(GCObjectHandle handle) {
    return std::make_unique<TableHolder>(handle);
}

} // effil
