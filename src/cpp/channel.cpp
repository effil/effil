#include "channel.h"

#include "lua-helpers.h"

#include <sol/sol.hpp>

namespace effil {

sol::usertype<Channel> Channel::exportAPI(sol::state_view& lua) {
    auto type = lua.new_usertype<Channel>(
        sol::no_constructor,
        sol::call_constructor, sol::factories(
            [](sol::this_state lua, const sol::object& capacity){
                return sol::make_object(lua, GC::instance().create<Channel>(capacity));
            },
            [](sol::this_state lua){
                return sol::make_object(lua, GC::instance().create<Channel>());
            }
        )
    );
    type["push"] = &Channel::push;
    type["pop"] = &Channel::pop;
    type["size"] = &Channel::size;
    return type;
}

void Channel::initialize(const sol::object& capacity) {
    if (capacity.valid()) {
        REQUIRE(capacity.get_type() == sol::type::number)
                << "bad argument #1 to 'effil.channel' (number expected, got "
                << luaTypename(capacity) << ")";
        REQUIRE(capacity.as<int>() >= 0)
                << "effil.channel: invalid capacity value = " << capacity.as<int>();
        ctx_->capacity_ = capacity.as<size_t>();
    }
    else {
        ctx_->capacity_ = 0;
    }
}

bool Channel::push(const sol::variadic_args& args) {
    if (!args.leftover_count())
        return false;

    std::unique_lock<std::mutex> lock(ctx_->lock_);
    if (ctx_->capacity_ && ctx_->channel_.size() >= ctx_->capacity_)
        return false;
    StoredArray array;
    for (const auto& arg : args) {
        try {
            auto obj = createStoredObject(arg.get<sol::object>());
            ctx_->addReference(obj->gcHandle());
            obj->releaseStrongReference();
            array.emplace_back(obj);
        }
        RETHROW_WITH_PREFIX("effil.channel:push");
    }
    ctx_->channel_.emplace(array);
    ctx_->cv_.notify_one();
    return true;
}

StoredArray Channel::pop(const sol::optional<int>& duration,
                          const sol::optional<std::string>& period) {
    this_thread::interruptionPoint();
    std::unique_lock<std::mutex> lock(ctx_->lock_);
    {
        this_thread::ScopedSetInterruptable interruptable(this);

        Timer timer(duration ? fromLuaTime(duration.value(), period) :
                               std::chrono::milliseconds());
        while (ctx_->channel_.empty()) {
            if (duration) {
                if (timer.isFinished() ||
                        ctx_->cv_.wait_for(lock, timer.left()) ==
                            std::cv_status::timeout) {
                    return StoredArray();
                }
            }
            else { // No time limit
                ctx_->cv_.wait(lock);
            }
            this_thread::interruptionPoint();
        }
    }

    auto ret = ctx_->channel_.front();
    for (const auto& obj: ret) {
        obj->holdStrongReference();
        ctx_->removeReference(obj->gcHandle());
    }

    ctx_->channel_.pop();
    return ret;
}

size_t Channel::size() {
    std::lock_guard<std::mutex> lock(ctx_->lock_);
    return ctx_->channel_.size();
}

void Channel::interrupt()
{
    ctx_->cv_.notify_all();
}

} // namespace effil
