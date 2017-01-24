#include "threading.h"

namespace effil {

LuaThread::LuaThread(const sol::function& function, const sol::variadic_args& args) noexcept {
    // 1. Dump function to string
    sol::state_view lua(function.lua_state());
    str_function_ = lua["string"]["dump"](function);

    // 2. Create new state
    p_state_.reset(new sol::state);
    ASSERT(p_state_.get() != NULL);
    p_state_->open_libraries(
        sol::lib::base, sol::lib::string,
        sol::lib::package, sol::lib::io, sol::lib::os
    );
    getUserType(*p_state_);
    effil::SharedTable::getUserType(*p_state_);

    // 3. Save parameters
    storeArgs(args);

    // 4. Run thread
    p_thread_.reset(new std::thread(&LuaThread::work, this));
    ASSERT(p_thread_.get() != NULL);
}

void LuaThread::storeArgs(const sol::variadic_args &args) noexcept {
    p_arguments_ = std::make_shared<std::vector<sol::object>>();
    for(auto iter = args.begin(); iter != args.end(); iter++) {
        effil::StoredObject store(iter->get<sol::object>());
        p_arguments_->push_back(store.unpack(sol::this_state{p_state_->lua_state()}));
    }
}

void LuaThread::join() noexcept {
    if (p_thread_.get()) {
        p_thread_->join();
        p_thread_.reset();
    }
    if (p_arguments_.get())
        p_arguments_.reset();
    if (p_state_.get())
        p_state_.reset();
}

void LuaThread::detach() noexcept {
    p_thread_->detach();
}

void LuaThread::work() noexcept  {
    ASSERT(p_state_.get() && p_arguments_.get()) << "invalid thread Lua state\n";

    std::string func_owner = std::move(str_function_);
    std::shared_ptr<sol::state> state_owner = p_state_;
    std::shared_ptr<std::vector<sol::object>> arguments_owner = p_arguments_;
    sol::function_result func = (*state_owner)["loadstring"](func_owner);
    func.get<sol::function>()(sol::as_args(*arguments_owner));
}

std::string LuaThread::threadId() noexcept {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

sol::object LuaThread::getUserType(sol::state_view &lua) noexcept
{
    static sol::usertype<LuaThread> type(
        sol::call_construction(), sol::constructors<sol::types<sol::function, sol::variadic_args>>(),
        "join",      &LuaThread::join,
        "detach",    &LuaThread::detach,
        "thread_id", &LuaThread::threadId
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

} // effil
