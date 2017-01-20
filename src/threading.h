#pragma once

#include "shared-table.h"

#include <sol.hpp>

#include <iostream>
#include <sstream>
#include <thread>

namespace threading {

class LuaThread
{
public:
    LuaThread(const sol::function& function, const sol::variadic_args& args) noexcept;
    virtual ~LuaThread() noexcept = default;
    void join() noexcept;

    static std::string thread_id() noexcept;
    static sol::object get_user_type(sol::state_view& lua) noexcept;

private:
    void work() noexcept;
    void store_args(const sol::variadic_args& args) noexcept;

    std::string str_function_;
    std::shared_ptr<sol::state> p_state_;
    std::shared_ptr<std::thread> p_thread_;
    std::vector<sol::object> arguments_;
};

}
