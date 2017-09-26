#pragma once

#include "gc-object.h"
#include "notifier.h"
#include "stored-object.h"
#include "lua-helpers.h"
#include "queue"

namespace effil {

class Channel : public GCObject {
public:
    Channel(sol::optional<int> capacity);
    static void exportAPI(sol::state_view& lua);

    bool push(const sol::variadic_args& args);
    StoredArray pop(const sol::optional<int>& duration,
                     const sol::optional<std::string>& period);

    size_t size();
protected:
    struct SharedData {
        std::mutex lock_;
        std::condition_variable cv_;
        size_t capacity_;
        std::queue<StoredArray> channel_;
    };

    std::shared_ptr<SharedData> data_;
};

} // namespace effil
