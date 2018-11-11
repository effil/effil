#include "clock.h"

#include <chrono>

namespace effil {
namespace clock {

namespace {

template <typename Clock, typename  Units>
uint64_t timeSinceEpoch() {
    return std::chrono::duration_cast<Units>(Clock::now().time_since_epoch()).count();
}

template <typename Clock>
sol::table makeClock(sol::state_view lua) {
    return lua.create_table_with(
            "s",  timeSinceEpoch<Clock, std::chrono::seconds>,
            "ms", timeSinceEpoch<Clock, std::chrono::milliseconds>,
            "ns", timeSinceEpoch<Clock, std::chrono::nanoseconds>
    );
}

} // namespace

sol::table exportAPI(sol::state_view lua) {
    return lua.create_table_with(
            "system", makeClock<std::chrono::system_clock>(lua),
            "steady", makeClock<std::chrono::system_clock>(lua));
}

} // clock
} // effil