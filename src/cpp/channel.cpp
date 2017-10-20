#include "channel.h"

#include "sol.hpp"

namespace effil {

void ChannelView::exportAPI(sol::state_view& lua) {
    sol::usertype<ChannelView> type("new", sol::no_constructor,
        "push",  &ChannelView::push,
        "pop",  &ChannelView::pop,
        "size", &ChannelView::size
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

ChannelView::ChannelView(const sol::stack_object& capacity) : View<ChannelImpl>() {
    if (capacity.valid()) {
        REQUIRE(capacity.get_type() == sol::type::number) << "bad argument #1 to 'effil.channel' (number expected, got "
                                                          << luaTypename(capacity) << ")";
        REQUIRE(capacity.as<int>() >= 0) << "effil.channel: invalid capacity value = " << capacity.as<int>();
        impl_->capacity_ = capacity.as<size_t>();
    }
    else {
        impl_->capacity_ = 0;
    }
}

bool ChannelView::push(const sol::variadic_args& args) {
    if (!args.leftover_count())
        return false;

    std::unique_lock<std::mutex> lock(impl_->lock_);
    if (impl_->capacity_ && impl_->channel_.size() >= impl_->capacity_)
        return false;
    effil::StoredArray array;
    for (const auto& arg : args) {
        try {
            auto obj = createStoredObject(arg.get<sol::object>());
            impl_->addReference(obj->gcHandle());
            obj->releaseStrongReference();
            array.emplace_back(obj);
        }
        RETHROW_WITH_PREFIX("effil.channel:push");
    }
    if (impl_->channel_.empty())
        impl_->cv_.notify_one();
    impl_->channel_.emplace(array);
    return true;
}

StoredArray ChannelView::pop(const sol::optional<int>& duration,
                          const sol::optional<std::string>& period) {
    std::unique_lock<std::mutex> lock(impl_->lock_);
    while (impl_->channel_.empty()) {
        if (duration) {
            if (impl_->cv_.wait_for(lock, fromLuaTime(duration.value(), period)) == std::cv_status::timeout)
                return StoredArray();
        }
        else { // No time limit
            impl_->cv_.wait(lock);
        }
    }

    auto ret = impl_->channel_.front();
    for (const auto& obj: ret) {
        obj->holdStrongReference();
        impl_->removeReference(obj->gcHandle());
    }

    impl_->channel_.pop();
    return ret;
}

size_t ChannelView::size() {
    std::lock_guard<std::mutex> lock(impl_->lock_);
    return impl_->channel_.size();
}

} // namespace effil
