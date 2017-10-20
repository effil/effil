#pragma once

#include "notifier.h"
#include "lua-helpers.h"
#include "impl.h"
#include "view.h"

#include <queue>

namespace effil {

class ChannelImpl : public BaseImpl {
public:
    std::mutex lock_;
    std::condition_variable cv_;
    size_t capacity_;
    std::queue<StoredArray> channel_;
};

class ChannelView : public View<ChannelImpl> {
public:
    static void exportAPI(sol::state_view& lua);

    bool push(const sol::variadic_args& args);
    StoredArray pop(const sol::optional<int>& duration,
                    const sol::optional<std::string>& period);

    size_t size();

public:
    ChannelView() = delete;

private:
    explicit ChannelView(const sol::stack_object& capacity);
    friend class GC;
};

} // namespace effil
