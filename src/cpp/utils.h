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
    Logger()
        : lockGuard_(lock_)
    {}

    ~Logger() {
        *stream_ << std::endl;
    }

    std::ostream& getStream() { return *stream_; }

private:
    static std::mutex lock_;
    static std::unique_ptr<std::ostream> stream_;

    std::lock_guard<std::mutex> lockGuard_;
};

std::string getCurrentTime();

} // effil

#ifdef NDEBUG
#   define DEBUG(x) if (false) std::cout
#else
#   define DEBUG(name) Logger().getStream() << getCurrentTime() \
                            << " " << "[" << std::this_thread::get_id() \
                            << "][" << name << "] "
#endif

#define REQUIRE(cond) if (!(cond)) throw effil::Exception()
#define RETHROW_WITH_PREFIX(preff) catch(const effil::Exception& err) { \
        DEBUG(preff) << err.what(); \
        throw effil::Exception() << preff << ": " << err.what(); \
    }
