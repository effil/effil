#pragma once

#include <iostream>
#include <sstream>

#include <sol.hpp>

namespace effil {
namespace utils {

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

} // utils
} // effil

#define ERROR throw effil::utils::Exception() << __FILE__ << ":" << __LINE__
#define ASSERT(cond) if (!(cond)) ERROR << "In condition '" << #cond << "': "
