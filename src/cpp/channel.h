#pragma once

#include "notifier.h"
#include "stored-object.h"
#include "lua-helpers.h"
#include "queue"

namespace effil {

class Channel : public GCObject {
public:
    Channel(sol::optional<int> capacity);
    static void getUserType(sol::state_view& lua);

    bool write(const sol::variadic_args& args);
    StoredArray read(const sol::optional<int>& duration,
                     const sol::optional<std::string>& period);

protected:
    struct SharedData {
        size_t capacity_;
        Notifier notifier_;
        std::queue<StoredArray> channel_;
        std::mutex lock_;
    };

    std::shared_ptr<SharedData> data_;
};

} // namespace effil
