#pragma once

#include <iostream>
#include <sstream>

#include <sol.hpp>

namespace utils {

class ExceptionThrower
{
public:
    void operator =(const std::ostream& os)
    {
        throw sol::error(static_cast<const std::stringstream&>(os).str());
    }

    operator bool()
    {
        return true;
    }

};

}

#define ERROR if (auto thr = utils::ExceptionThrower()) thr = std::stringstream() << __FILE__ << ":" << __LINE__ << std::endl
#define ASSERT(cond) if (!(cond)) ERROR << "In condition '" << #cond << "': "

