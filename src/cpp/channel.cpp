#include "channel.h"

#include "sol.hpp"

namespace effil {

void Channel::getUserType(sol::state_view& lua) {
    sol::usertype<Channel> type("new", sol::no_constructor,
        "write",  &Channel::write,
        "read",  &Channel::read
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

Channel::Channel(sol::optional<int> capacity) : capacity_(capacity.value_or(0)) {
    REQUIRE(capacity_ >= 0) << "Invalid capacity value = " << capacity.value();
}

bool Channel::write(const sol::variadic_args& args) {
    std::unique_lock<std::mutex> lock(lock_);
    if (channel_.size() >= (size_t)capacity_)
        return false;
    effil::StoredArray array;
    for (const auto& arg : args) {
        array.emplace_back(createStoredObject(arg.get<sol::object>()));
    }
    channel_.emplace(array);
    return true;
}

StoredArray Channel::read() {
    std::unique_lock<std::mutex> lock(lock_);
    auto ret = channel_.front();
    channel_.pop();
    return std::move(ret);
}

}
