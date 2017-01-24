#pragma once

#include "shared-table.h"

#include <iostream>
#include <sstream>
#include <thread>

namespace effil {

class LuaThread {
public:
    LuaThread(const sol::function& function, const sol::variadic_args& args) noexcept;
    virtual ~LuaThread() noexcept = default;
    void join() noexcept;
    void detach() noexcept;

    static std::string threadId() noexcept;
    static sol::object getUserType(sol::state_view &lua) noexcept;

private:
    void work() noexcept;
    void storeArgs(const sol::variadic_args &args) noexcept;

    std::string str_function_;
    std::shared_ptr<sol::state> p_state_;
    std::shared_ptr<std::thread> p_thread_;
    std::shared_ptr<std::vector<sol::object>> p_arguments_;
};

} // effil
