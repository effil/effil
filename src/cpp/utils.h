#pragma once

#include <iostream>
#include <sstream>
#include <thread>
#include <ctime>
#include <mutex>
#include <sol.hpp>


#if LUA_VERSION_NUM < 501 || LUA_VERSION_NUM > 503
#   error Unsupported Lua version
#endif

#if LUA_VERSION_NUM == 503
#   define LUA_INDEX_TYPE lua_Integer
#else
#   define LUA_INDEX_TYPE lua_Number
#endif

extern "C"
#ifdef _WIN32
 __declspec(dllexport)
#endif
int luaopen_effil(lua_State* L);

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

class Logger
{
public:
    Logger(std::ostream& s, std::mutex& m)
        : stream_(s), lock_(m)
    {}

    Logger(Logger&&) = default;

    ~Logger() {
        stream_ << std::endl;
    }

    std::ostream& get() { return stream_; }

private:
    std::ostream& stream_;
    std::unique_lock<std::mutex> lock_;
};

Logger getLogger();
std::string getCurrentTime();

} // effil

#ifdef NDEBUG
#   define DEBUG(x) if (false) std::cout
#else
#   define DEBUG(name) getLogger().get() << getCurrentTime() << " " << \
                       "[" << std::this_thread::get_id() << "][" << name << "] "
#endif

#define REQUIRE(cond) if (!(cond)) throw effil::Exception()
#define RETHROW_WITH_PREFIX(preff) catch(const effil::Exception& err) { \
        DEBUG(preff) << err.what(); \
        throw effil::Exception() << preff << ": " << err.what(); \
    }
