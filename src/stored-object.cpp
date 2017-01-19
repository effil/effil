#include "stored-object.h"

#include "shared-table.h"

#include <map>
#include <vector>

#include <cassert>

namespace share_data {

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

}
