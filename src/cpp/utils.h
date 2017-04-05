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

} // effil

#define REQUIRE(cond) if (!(cond)) throw effil::Exception()
#define NDEBUG
#ifdef NDEBUG
#define DEBUG if (false) std::cout
#else
#define DEBUG std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << " tid:" << std::this_thread::get_id() << " "
#endif
