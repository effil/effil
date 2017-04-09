#pragma once

#include "stored-object.h"
#include "lua-helpers.h"
#include "queue"

namespace effil {

class Channel {
public:
    Channel(sol::optional<int> capacity);
    static void getUserType(sol::state_view& lua);

    bool write(const sol::variadic_args& args);
    StoredArray read();

protected:
    int capacity_;
    std::queue<StoredArray> channel_;
    std::mutex lock_;
};

} // namespace effil
