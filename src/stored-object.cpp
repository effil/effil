#include "stored-object.h"

#include "shared-table.h"
#include "utils.h"

#include <map>
#include <vector>

#include <cassert>

namespace share_data {

namespace {

template<typename StoredType>
class PrimitiveHolder : public BaseHolder {
public:
    PrimitiveHolder(sol::stack_object luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {}

    PrimitiveHolder(sol::object luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {}

    PrimitiveHolder(const StoredType& init) noexcept
            : data_(init) {}

    bool rawCompare(const BaseHolder* other) const noexcept final {
        assert(type_ == other->type());
        return static_cast<const PrimitiveHolder<StoredType>*>(other)->data_ == data_;
    }

    bool rawLess(const BaseHolder* other) const noexcept final {
        assert(type_ == other->type());
        return data_ < static_cast<const PrimitiveHolder<StoredType>*>(other)->data_;
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

        assert(dumper.valid());
        function_ = dumper(luaObject);
    }

    bool rawCompare(const BaseHolder* other) const noexcept final {
        return function_ == static_cast<const FunctionHolder*>(other)->function_;
    }

    bool rawLess(const BaseHolder* other) const noexcept final {
        return function_ < static_cast<const FunctionHolder*>(other)->function_;
    }

    std::size_t hash() const noexcept final {
        return std::hash<std::string>()(function_);
    }

    sol::object unpack(sol::this_state state) const noexcept final {
        sol::state_view lua((lua_State*)state);
        sol::function loader = lua["loadstring"];
        assert(loader.valid());

        sol::function result = loader(function_);
        if (!result.valid()) {
            ERROR << "Unable to restore function!" << std::endl;
            ERROR << "Content:" << std::endl;
            ERROR << function_ << std::endl;
        }
        return sol::make_object(state, result);
    }

private:
    std::string function_;
};

// This class is used as a storage for visited sol::tables
// TODO: try to use map or unordered map instead of linersearch in vectoers
// TODO: Trick is - sol::object has only operator==:/
class VisitedTables {
public:
    VisitedTables() = default;
    void add(sol::table key, SharedTable* value) noexcept {
        keys.push_back(key);
        values.push_back(value);
    }

    SharedTable* get(sol::table key) const noexcept {
        for(size_t i = 0; i < keys.size(); i++) {
            if (keys[i] == key) {
                return values[i];
            }
        }
        return nullptr;
    }

private:
    std::vector<sol::table> keys;
    std::vector<SharedTable*> values;
private:
    VisitedTables(const VisitedTables&) = delete;
};

void dumpTable(SharedTable* target, sol::table luaTable, VisitedTables& visited) noexcept;

StoredObject makeStoredObject(sol::object luaObject, VisitedTables& visited) noexcept {
    if (luaObject.get_type() == sol::type::table) {
        sol::table luaTable = luaObject;
        auto st = visited.get(luaTable);
        if (st == nullptr) {
            SharedTable* table = defaultPool().getNew();
            visited.add(luaTable, table);
            dumpTable(table, luaTable, visited);
            return StoredObject(table);
        } else {
            return StoredObject(st);
        }
    } else {
        return StoredObject(luaObject);
    }
}

void dumpTable(SharedTable* target, sol::table luaTable, VisitedTables& visited) noexcept {
    for(auto& row : luaTable) {
        target->set(makeStoredObject(row.first, visited), makeStoredObject(row.second, visited));
    }
}

template<typename SolObject>
std::unique_ptr<BaseHolder> fromSolObject(SolObject luaObject) {
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
            VisitedTables visited;
            visited.add(luaTable, table);

            // Let's dump table and all subtables
            // VisitedTables is used to prevent from infinity recursion
            // in recursive tables
            dumpTable(table, luaTable, visited);
            return std::make_unique<PrimitiveHolder<SharedTable*>>(table);
        }
        default:
            ERROR << "Unable to store object of that type: " << (int)luaObject.get_type() << std::endl;
    }
    return nullptr;
}

} // namespace

StoredObject::StoredObject(StoredObject&& init) noexcept
        : data_(std::move(init.data_)) {}

StoredObject::StoredObject(SharedTable* table) noexcept
        : data_(new PrimitiveHolder<SharedTable*>(table)) {
}

StoredObject::StoredObject(sol::object object) noexcept
        : data_(fromSolObject(object)) {
}

StoredObject::StoredObject(sol::stack_object object) noexcept
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
    if (data_)
        return data_->compare(o.data_.get());
    else
        return data_.get() == o.data_.get();
}

bool StoredObject::operator<(const StoredObject& o) const noexcept {
    if (data_)
        return data_->less(o.data_.get());
    else
        return data_.get() < o.data_.get();
}

} // share_data
