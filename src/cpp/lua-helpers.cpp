#include "lua-helpers.h"

namespace effil {

std::chrono::milliseconds fromLuaTime(int duration, const sol::optional<std::string>& period) {
    using namespace std::chrono;

    REQUIRE(duration >= 0) << "Invalid duration interval: " << duration;

    std::string metric = period ? period.value() : "s";
    if (metric == "ms") return milliseconds(duration);
    else if (metric == "s") return seconds(duration);
    else if (metric == "m") return minutes(duration);
    else throw sol::error("invalid time identification: " + metric);
}

}
