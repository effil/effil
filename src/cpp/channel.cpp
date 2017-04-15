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

Channel::Channel(sol::optional<int> capacity) : data_(std::make_shared<SharedData>()){
    if (capacity) {
        REQUIRE(capacity.value() >= 0) << "Invalid capacity value = " << capacity.value();
        data_->capacity_ = static_cast<size_t>(capacity.value());
    }
    else {
        data_->capacity_ = 0;
    }
}

bool Channel::write(const sol::variadic_args& args) {
    if (!args.leftover_count())
        return false;

    std::unique_lock<std::mutex> lock(data_->lock_);
    if (data_->capacity_ && data_->channel_.size() >= data_->capacity_)
        return false;
    effil::StoredArray array;
    for (const auto& arg : args) {
        auto obj = createStoredObject(arg.get<sol::object>());
        if (obj->gcHandle())
            refs_->insert(obj->gcHandle());
        array.emplace_back(obj);
    }
    if (data_->channel_.empty())
        data_->notifier_.notify();
    data_->channel_.emplace(array);
    return true;
}

StoredArray Channel::read(const sol::optional<int>& duration,
                          const sol::optional<std::string>& period) {
    bool waitUnlimitedly = !duration;
    std::unique_lock<std::mutex> lock(data_->lock_);
    bool isEmpty = !data_->channel_.size();
    if (isEmpty)
    {
        do {
            lock.unlock();
            if (waitUnlimitedly)
                data_->notifier_.wait();
            else
                data_->notifier_.waitFor(fromLuaTime(duration.value(), period));
            lock.lock();
            isEmpty = !data_->channel_.size();
        } while (isEmpty && waitUnlimitedly);
    }
    if (isEmpty)
        return StoredArray();

    auto ret = data_->channel_.front();
    for (const auto& obj: ret) {
        if (obj->gcHandle())
            refs_->erase(obj->gcHandle());
    }
    data_->channel_.pop();
    if (!data_->channel_.size())
        data_->notifier_.reset();
    return ret;
}

}
