#include "stored-object.h"

#include "shared-table.h"
#include "utils.h"

#include <map>
#include <vector>
#include <algorithm>

#include <cassert>

namespace effil {

namespace {

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

class FunctionHolder : public BaseHolder {
public:
    template <typename SolObject>
    FunctionHolder(SolObject luaObject) noexcept {
        sol::state_view lua(luaObject.lua_state());
        sol::function dumper = lua["string"]["dump"];
        if (!dumper.valid())
            throw Exception() << "Invalid string.dump()";
        function_ = dumper(luaObject);
    }

    bool rawCompare(const BaseHolder* other) const noexcept final {
        return function_ < static_cast<const FunctionHolder*>(other)->function_;
    }

    sol::object unpack(sol::this_state state) const final {
        sol::state_view lua((lua_State*)state);
        sol::function loader = lua["loadstring"];
        REQUIRE(loader.valid()) << "Invalid loadstring()";
        sol::function result = loader(function_);
        // The result of restaring always is valid function.
        assert(result.valid());
        return sol::make_object(state, result);
    }

private:
    std::string function_;
};

class TableHolder : public BaseHolder {
public:
    template <typename SolType>
    TableHolder(const SolType& luaObject) {
        assert(luaObject.template is<SharedTable>());
        handle_ = luaObject.template as<SharedTable>().handle();
        assert(getGC().has(handle_));
    }

    TableHolder(GCObjectHandle handle)
            : handle_(handle) {}

    bool rawCompare(const BaseHolder* other) const final {
        return handle_ < static_cast<const TableHolder*>(other)->handle_;
    }

    sol::object unpack(sol::this_state state) const final {
        return sol::make_object(state, *static_cast<SharedTable*>(getGC().get(handle_)));
    }

    GCObjectHandle gcHandle() const override { return handle_; }

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
        auto comparator = [&luaTable](const std::pair<sol::table, GCObjectHandle>& element) {
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
    for (auto& row : luaTable) {
        target->set(makeStoredObject(row.first, visited), makeStoredObject(row.second, visited));
    }
}

template <typename SolObject>
StoredObject fromSolObject(const SolObject& luaObject) {
    switch (luaObject.get_type()) {
        case sol::type::nil:
            return std::make_unique<NilHolder>();
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
        case sol::type::table: {
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
            throw Exception() << "Unable to store object of that type: " << (int)luaObject.get_type() << "\n";
    }
    return nullptr;
}

} // namespace

StoredObject createStoredObject(bool value) { return std::make_unique<PrimitiveHolder<bool>>(value); }

StoredObject createStoredObject(double value) { return std::make_unique<PrimitiveHolder<double>>(value); }

StoredObject createStoredObject(const std::string& value) {
    return std::make_unique<PrimitiveHolder<std::string>>(value);
}

StoredObject createStoredObject(const char* value) {
    return std::make_unique<PrimitiveHolder<std::string>>(value);
}

StoredObject createStoredObject(const sol::object& object) { return fromSolObject(object); }

StoredObject createStoredObject(const sol::stack_object& object) { return fromSolObject(object); }

StoredObject createStoredObject(GCObjectHandle handle) { return std::make_unique<TableHolder>(handle); }

template <typename DataType>
sol::optional<DataType> getPrimitiveHolderData(const StoredObject& sobj) {
    auto ptr = dynamic_cast<PrimitiveHolder<DataType>*>(sobj.get());
    if (ptr)
        return ptr->getData();
    return sol::optional<DataType>();
}

sol::optional<bool> storedObjectToBool(const StoredObject& sobj) { return getPrimitiveHolderData<bool>(sobj); }

sol::optional<double> storedObjectToDouble(const StoredObject& sobj) { return getPrimitiveHolderData<double>(sobj); }

sol::optional<std::string> storedObjectToString(const StoredObject& sobj) {
    return getPrimitiveHolderData<std::string>(sobj);
}

} // effil
