#pragma once

#include <sol.hpp>
#include <iostream>
#include <sstream>
#include <thread>

#include "shared-table.h"

class LuaThread
{
public:
    LuaThread(const sol::function& function, const sol::variadic_args& args) noexcept;
    virtual ~LuaThread() noexcept;
    void join() noexcept;
    static std::string thread_id() noexcept;

private:
    void work() noexcept;
    void validate_args(const sol::variadic_args& args) noexcept;

    std::string str_function_;
    std::shared_ptr<sol::state> p_state_;
    std::shared_ptr<std::thread> p_thread_;
    std::vector<sol::object> arguments_;
};
