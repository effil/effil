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

Channel::Channel(sol::optional<int> capacity) : data_(std::make_shared<SharedData>()){
    if (capacity) {
        REQUIRE(capacity.value() >= 0) << "Invalid capacity value = " << capacity.value();
        data_->capacity_ = static_cast<size_t>(capacity.value());
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
        auto obj = createStoredObject(arg.get<sol::object>());
        if (obj->gcHandle())
            data_->refs_.insert(obj->gcHandle());
        obj->releaseStrongReference();
        array.emplace_back(obj);
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
    for (const auto& obj: ret) {
        if (obj->gcHandle())
            data_->refs_.erase(obj->gcHandle());
    }
    data_->channel_.pop();
    return ret;
}

size_t Channel::size() {
    std::lock_guard<std::mutex> lock(data_->lock_);
    return data_->channel_.size();
}

GCObjectHandle Channel::handle() const {
    return reinterpret_cast<GCObjectHandle>(data_.get());
}

size_t Channel::instances() const {
    return data_.use_count();
}

const std::unordered_set<GCObjectHandle>& Channel::refers() const {
    return data_->refs_;
}

} // namespace effil
