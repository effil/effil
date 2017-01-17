#include "stored-object.h"

#include "shared-table.h"

#include <map>
#include <vector>
#include <iostream>

#include <cassert>

#define ERROR std::cerr

namespace core {

FunctionHolder::FunctionHolder(sol::stack_object luaObject) noexcept {
    sol::state_view lua(luaObject.lua_state());
    sol::function dumper = lua["string"]["dump"];

    assert(dumper.valid());
    function_ = dumper(luaObject);
}

bool FunctionHolder::rawCompare(const BaseHolder* other) const noexcept {
    return function_ == static_cast<const FunctionHolder*>(other)->function_;
}

bool FunctionHolder::rawLess(const BaseHolder* other) const noexcept {
    return function_ < static_cast<const FunctionHolder*>(other)->function_;
}

std::size_t FunctionHolder::hash() const noexcept {
    return std::hash<std::string>()(function_);
}

sol::object FunctionHolder::unpack(sol::this_state state) const noexcept {
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

StoredObject::StoredObject(StoredObject&& init) noexcept
        : data_(std::move(init.data_)) {}

StoredObject::StoredObject(sol::stack_object luaObject) noexcept
        : data_(nullptr) {
    switch(luaObject.get_type()) {
        case sol::type::nil:
            break;
        case sol::type::boolean:
            data_.reset(new PrimitiveHolder<bool>(luaObject));
            break;
        case sol::type::number:
            data_.reset(new PrimitiveHolder<double>(luaObject));
            break;
        case sol::type::string:
            data_.reset(new PrimitiveHolder<std::string>(luaObject));
            break;
        case sol::type::userdata:
            data_.reset(new PrimitiveHolder<SharedTable*>(luaObject));
            break;
        case sol::type::function:
            data_.reset(new FunctionHolder(luaObject));
            break;
        default:
            ERROR << "Unable to store object of that type: " << (int)luaObject.get_type() << std::endl;
    }
}

StoredObject::operator bool() const noexcept {
    return (bool)data_;
}

StoredObject::StoredObject(SharedTable* table) noexcept
        : data_(new PrimitiveHolder<SharedTable*>(table)) {
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

} // core