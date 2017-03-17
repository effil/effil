#pragma once

#include <iostream>
#include <sstream>
#include <thread>

#include <sol.hpp>

namespace effil {

class Exception : public sol::error {
public:
    Exception() noexcept
            : sol::error("") {}

    template <typename T>
    Exception& operator<<(const T& value) {
        std::stringstream ss;
        ss << value;
        message_ += ss.str();
        return *this;
    }

    virtual const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

inline void openAllLibs(sol::state& lua) {
    lua.open_libraries(sol::lib::base,
        sol::lib::string,
        sol::lib::package,
        sol::lib::io,
        sol::lib::os,
        sol::lib::count,
        sol::lib::bit32,
        sol::lib::coroutine,
        sol::lib::debug,
        sol::lib::ffi,
        sol::lib::jit,
        sol::lib::math,
        sol::lib::utf8,
        sol::lib::table);
}

} // effil

#define REQUIRE(cond) if (!(cond)) throw effil::Exception()

#ifdef NDEBUG
#define DEBUG if (false) std::cout
#else
#define DEBUG std::cout << __FUNCTION__ << "::" << __LINE__ << " <" << std::this_thread::get_id() << ">: "
#endif
