#pragma once

#include <iostream>
#include <sstream>
#include <thread>

#include <sol.hpp>

#if LUA_VERSION_NUM < 501 || LUA_VERSION_NUM > 503
#   error Unsupported Lua version
#endif

#if LUA_VERSION_NUM == 503
#   define LUA_INDEX_TYPE lua_Integer
#else
#   define LUA_INDEX_TYPE lua_Number
#endif

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

class ScopeGuard {
public:
    ScopeGuard(const std::function<void()>& f) : f_(f) {}
    ~ScopeGuard() { f_(); }

private:
    std::function<void()> f_;
};

} // effil

#define REQUIRE(cond) if (!(cond)) throw effil::Exception()
#define RETHROW_WITH_PREFIX(preff) catch(const effil::Exception& err) { throw effil::Exception() << preff << ": " << err.what(); }

#ifdef NDEBUG
#define DEBUG if (false) std::cout
#else
#define DEBUG std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << " tid:" << std::this_thread::get_id() << " "
#endif
