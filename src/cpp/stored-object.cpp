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

    bool compare(const BaseHolder* other) const noexcept final {
        return BaseHolder::compare(other) && static_cast<const PrimitiveHolder<StoredType>*>(other)->data_ == data_;
    }

    std::size_t hash() const noexcept final {
        return std::hash<StoredType>()(data_);
    }

    sol::object unpack(sol::this_state state) const noexcept final {
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

    bool compare(const BaseHolder* other) const noexcept final {
        return BaseHolder::compare(other) && static_cast<const FunctionHolder*>(other)->function_ == function_;
    }

    std::size_t hash() const noexcept final {
        return std::hash<std::string>()(function_);
    }

    sol::object unpack(sol::this_state state) const noexcept final {
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

// This class is used as a storage for visited sol::tables
// TODO: try to use map or unordered map instead of linear search in vector
// TODO: Trick is - sol::object has only operator==:/
typedef std::vector<std::pair<sol::object, SharedTable*>> SolTableToShared;

void dumpTable(SharedTable* target, sol::table luaTable, SolTableToShared& visited) noexcept;

StoredObject makeStoredObject(sol::object luaObject, SolTableToShared& visited) noexcept {
    if (luaObject.get_type() == sol::type::table) {
        sol::table luaTable = luaObject;
        auto comparator = [&luaTable](const std::pair<sol::table, SharedTable*>& element){
            return element.first == luaTable;
        };
        auto st = std::find_if(visited.begin(), visited.end(), comparator);

        if (st == std::end(visited)) {
            SharedTable* table = defaultPool().getNew();
            visited.emplace_back(std::make_pair(luaTable, table));
            dumpTable(table, luaTable, visited);
            return StoredObject(table);
        } else {
            return StoredObject(st->second);
        }
    } else {
        return StoredObject(luaObject);
    }
}

void dumpTable(SharedTable* target, sol::table luaTable, SolTableToShared& visited) noexcept {
    for(auto& row : luaTable) {
        target->set(makeStoredObject(row.first, visited), makeStoredObject(row.second, visited));
    }
}

template<typename SolObject>
std::unique_ptr<BaseHolder> fromSolObject(const SolObject& luaObject) {
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
            return std::make_unique<PrimitiveHolder<SharedTable*>>(luaObject);
        case sol::type::function:
            return std::make_unique<FunctionHolder>(luaObject);
        case sol::type::table:
        {
            sol::table luaTable = luaObject;
            // Tables pool is used to store tables.
            // Right now not defiantly clear how ownership between states works.
            SharedTable* table = defaultPool().getNew();
            SolTableToShared visited{{luaTable, table}};

            // Let's dump table and all subtables
            // SolTableToShared is used to prevent from infinity recursion
            // in recursive tables
            dumpTable(table, luaTable, visited);
            return std::make_unique<PrimitiveHolder<SharedTable*>>(table);
        }
        default:
            ERROR << "Unable to store object of that type: " << (int)luaObject.get_type() << "\n";
    }
    return nullptr;
}

} // namespace

StoredObject::StoredObject(StoredObject&& init) noexcept
        : data_(std::move(init.data_)) {}

StoredObject::StoredObject(SharedTable* table) noexcept
        : data_(new PrimitiveHolder<SharedTable*>(table)) {
}

StoredObject::StoredObject(const sol::object& object) noexcept
        : data_(fromSolObject(object)) {
}

StoredObject::StoredObject(const sol::stack_object& object) noexcept
        : data_(fromSolObject(object)) {
}

StoredObject::operator bool() const noexcept {
    return (bool)data_;
}

std::size_t StoredObject::hash() const noexcept {
    if (data_)
        return data_->hash();
    else
        return 0;
}

sol::object StoredObject::unpack(sol::this_state state) const noexcept {
    if (data_)
        return data_->unpack(state);
    else
        return sol::nil;
}

StoredObject& StoredObject::operator=(StoredObject&& o) noexcept {
    data_ = std::move(o.data_);
    return *this;
}

bool StoredObject::operator==(const StoredObject& o) const noexcept {
    if (data_ && o.data_)
        return data_->compare(o.data_.get());
    else
        return data_.get() == o.data_.get();
}

} // effil
