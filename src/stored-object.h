#pragma once

#include <sol.hpp>

#include <utility>

namespace core {

class BaseHolder {
public:
    BaseHolder() noexcept : type_(sol::type::nil) {}
    virtual ~BaseHolder() = default;

    sol::type type() const noexcept {
        return type_;
    }

    bool compare(const BaseHolder* other) const noexcept {
        assert(other != nullptr);
        return type_ == other->type_ && rawCompare(other);
    }
    virtual bool less(const BaseHolder* other) const noexcept {
        assert(other != nullptr);
        return type_ < other->type_ && rawLess(other);
    }

    virtual bool rawCompare(const BaseHolder* other) const noexcept = 0;
    virtual bool rawLess(const BaseHolder* other) const noexcept = 0;
    virtual std::size_t hash() const noexcept = 0;
    virtual sol::object unpack(sol::this_state state) const noexcept = 0;

protected:
    sol::type type_;

private:
    BaseHolder(const BaseHolder&) = delete;
    BaseHolder(BaseHolder&) = delete;
};

template<typename StoredType>
class PrimitiveHolder : public BaseHolder {
public:
    PrimitiveHolder(sol::stack_object luaObject) noexcept
            : data_(luaObject.as<StoredType>()) {
    }

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
    FunctionHolder(sol::stack_object luaObject) noexcept;
    bool rawCompare(const BaseHolder* other) const noexcept final;
    bool rawLess(const BaseHolder* other) const noexcept final;
    std::size_t hash() const noexcept final;
    sol::object unpack(sol::this_state state) const noexcept final;

private:
    std::string function_;
};

class SharedTable;

class StoredObject {
public:
    StoredObject() = default;
    StoredObject(StoredObject&& init) noexcept;
    StoredObject(sol::stack_object luaObject) noexcept;
    //StoredObject(sol::object luaObject) noexcept;
    StoredObject(SharedTable*) noexcept;

    operator bool() const noexcept;
    std::size_t hash() const noexcept;
    sol::object unpack(sol::this_state state) const noexcept;
    StoredObject& operator=(StoredObject&& o) noexcept;
    bool operator==(const StoredObject& o) const noexcept;
    bool operator<(const StoredObject& o) const noexcept;

private:
    std::unique_ptr<BaseHolder> data_;

private:
    StoredObject(const StoredObject&) = delete;
    StoredObject& operator=(const StoredObject&) = delete;
};

} // core

namespace std {

template<>
struct hash<core::StoredObject> {
    std::size_t operator()(const core::StoredObject &object) const noexcept {
        return object.hash();
    }
};

} // std