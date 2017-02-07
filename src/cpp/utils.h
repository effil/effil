#pragma once

#include <iostream>
#include <sstream>

#include <sol.hpp>

namespace effil {

class Exception : public sol::error {
public:
    Exception() noexcept : sol::error("") {}

    template<typename T>
    Exception& operator<<(const T& value) {
        std::stringstream ss;
        ss << value;
        message_ += ss.str();
        return *this;
    }

    virtual const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

} // effil

#define ERROR throw effil::Exception() << __FILE__ << ":" << __LINE__
#define ASSERT(cond) if (!(cond)) ERROR << "In condition '" << #cond << "': "

#ifdef NDEBUG
#    define DEBUG if (false) std::cout
#else
#    define DEBUG std::cout
#endif
