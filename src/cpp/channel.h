#pragma once

#include "notifier.h"
#include "gc-data.h"
#include "gc-object.h"

#include <queue>

namespace effil {

class ChannelData : public GCData {
public:
    std::mutex lock_;
    std::condition_variable cv_;
    size_t capacity_;
    std::queue<StoredArray> channel_;
};

class Channel : public GCObject<ChannelData>, public IInterruptable {
public:
    static sol::usertype<Channel> exportAPI(sol::state_view& lua);

    bool push(const sol::variadic_args& args);
    StoredArray pop(const sol::optional<int>& duration,
                    const sol::optional<std::string>& period);

    size_t size();

    void interrupt() final;

private:
    Channel() = default;
    void initialize(const sol::object& capacity = {});
    friend class GC;
};

} // namespace effil
