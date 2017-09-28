#include "channel.h"

#include "sol.hpp"

namespace effil {

void Channel::exportAPI(sol::state_view& lua) {
    sol::usertype<Channel> type("new", sol::no_constructor,
        "push",  &Channel::push,
        "pop",  &Channel::pop,
        "size", &Channel::size
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

Channel::Channel(const sol::stack_object& capacity) : data_(std::make_shared<SharedData>()){
    if (capacity.valid()) {
        REQUIRE(capacity.get_type() == sol::type::number) << "bad argument #1 to 'effil.channel' (number expected, got "
                                                          << luaTypename(capacity) << ")";
        REQUIRE(capacity.as<int>() >= 0) << "effil.channel: invalid capacity value = " << capacity.as<int>();
        data_->capacity_ = capacity.as<size_t>();
    }
    else {
        data_->capacity_ = 0;
    }
}

bool Channel::push(const sol::variadic_args& args) {
    if (!args.leftover_count())
        return false;

    std::unique_lock<std::mutex> lock(data_->lock_);
    if (data_->capacity_ && data_->channel_.size() >= data_->capacity_)
        return false;
    effil::StoredArray array;
    for (const auto& arg : args) {
        try {
            auto obj = createStoredObject(arg.get<sol::object>());
            addReference(obj->gcHandle());
            obj->releaseStrongReference();
            array.emplace_back(obj);
        }
        RETHROW_WITH_PREFIX("effil.channel:push");
    }
    if (data_->channel_.empty())
        data_->cv_.notify_one();
    data_->channel_.emplace(array);
    return true;
}

StoredArray Channel::pop(const sol::optional<int>& duration,
                          const sol::optional<std::string>& period) {
    std::unique_lock<std::mutex> lock(data_->lock_);
    while (data_->channel_.empty()) {
        if (duration) {
            if (data_->cv_.wait_for(lock, fromLuaTime(duration.value(), period)) == std::cv_status::timeout)
                return StoredArray();
        }
        else { // No time limit
            data_->cv_.wait(lock);
        }
    }

    auto ret = data_->channel_.front();
    for (const auto& obj: ret)
        removeReference(obj->gcHandle());

    data_->channel_.pop();
    return ret;
}

size_t Channel::size() {
    std::lock_guard<std::mutex> lock(data_->lock_);
    return data_->channel_.size();
}

} // namespace effil
