#include "threading.h"
#include "stored-object.h"

#include "utils.h"

namespace effil {

class LuaHookStopException : public std::exception {};

std::string threadId() noexcept
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void yield() noexcept
{
    std::this_thread::yield();
}

void sleep(int64_t time, sol::optional<std::string> period) noexcept
{
    std::string metric = period ? period.value() : "s";
    if (metric == "ms")
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    else if (metric == "s")
        std::this_thread::sleep_for(std::chrono::seconds(time));
    else if (metric == "m")
        std::this_thread::sleep_for(std::chrono::minutes(time));
    else
        throw sol::error("invalid time identificator: " + metric);
}

thread_local LuaThread::ThreadData* LuaThread::pThreadLocalData = NULL;

// class LuaThread

LuaThread::LuaThread(std::shared_ptr<ThreadData> threadData, const std::string& function, const sol::variadic_args& args) {
    pThreadData_ = threadData;
    ASSERT(pThreadData_.get());
    pThreadData_->command = ThreadCommand::Nothing;
    pThreadData_->status = ThreadStatus::Running;

    std::vector<sol::object> arguments;
    for(const auto& iter: args) {
        StoredObject store = createStoredObject(iter.get<sol::object>());
        arguments.push_back(store->unpack(sol::this_state{pThreadData_->luaState}));
    }

    pThread_.reset(new std::thread(&LuaThread::work, pThreadData_, function, std::move(arguments)));
    ASSERT(pThread_.get() != nullptr);
    pThread_->detach();
}

void LuaThread::luaHook(lua_State*, lua_Debug*)
{
    if (pThreadLocalData)
    {
        switch (pThreadLocalData->command)
        {
        case ThreadCommand::Pause:
        {
            pThreadLocalData->status = ThreadStatus::Paused;
            ThreadCommand cmd = pThreadLocalData->command;
            while (cmd == ThreadCommand::Pause) {
                std::this_thread::yield();
                cmd = pThreadLocalData->command;
            }
            assert(cmd != ThreadCommand::Nothing);
            if (cmd == ThreadCommand::Resume)
            {
                pThreadLocalData->status = ThreadStatus::Running;
                break; // Just go out of the function
            }
            else { /* HOOK_STOP - do nothing and go to the next case */}
        }
        case ThreadCommand::Cancel:
            throw LuaHookStopException();
        default:
        case ThreadCommand::Nothing:
            break;
        }
    }
}

void LuaThread::work(std::shared_ptr<ThreadData> threadData, const std::string strFunction, std::vector<sol::object>&& arguments) noexcept {
    try {
        pThreadLocalData = threadData.get();
        ASSERT(threadData.get()) << "invalid internal thread state\n";
        const sol::object& stringLoader = threadData->luaState["loadstring"];
        ASSERT(stringLoader.valid() && stringLoader.get_type() == sol::type::function);
        sol::function userFuncObj = static_cast<const sol::function&>(stringLoader)(strFunction);
        sol::function_result results = userFuncObj(sol::as_args(arguments));
        (void)results; // TODO: try to avoid use of useless sol::function_result here
        sol::variadic_args args(threadData->luaState, -lua_gettop(threadData->luaState));
        for(const auto& iter: args) {
            StoredObject store = createStoredObject(iter.get<sol::object>());
            threadData->results.emplace_back(std::move(store));
        }
        threadData->status = ThreadStatus::Completed;
    }
    catch (const LuaHookStopException&) {
        threadData->status = ThreadStatus::Canceled;
    }
    catch (const sol::error& err) {
        threadData->status = ThreadStatus::Failed;
        sol::stack::push(threadData->luaState, err.what());
        StoredObject store = createStoredObject(sol::stack::pop<sol::object>(threadData->luaState));
        threadData->results.emplace_back(std::move(store));
    }
}

void LuaThread::cancel() noexcept
{
    pThreadData_->command = ThreadCommand::Cancel;
}

void LuaThread::pause() noexcept
{
    pThreadData_->command = ThreadCommand::Pause;
}

void LuaThread::resume() noexcept
{
    pThreadData_->command = ThreadCommand::Resume;
}

std::tuple<sol::object, sol::table> LuaThread::wait(sol::this_state state) const noexcept
{

    ThreadStatus stat = pThreadData_->status;
    while (stat == ThreadStatus::Running) {
        std::this_thread::yield();
        stat = pThreadData_->status;
    }
    sol::table returns = sol::state_view(state).create_table();
    if (stat == ThreadStatus::Completed)
    {
        for (const StoredObject& obj: pThreadData_->results)
        {
            returns.add(obj->unpack(state));
        }
    }
    return std::make_tuple(sol::make_object(state, threadStatusToString(stat)), std::move(returns));
}

std::string LuaThread::threadStatusToString(ThreadStatus stat) const noexcept
{
    switch(stat)
    {
        case ThreadStatus::Running: return "running";
        case ThreadStatus::Paused: return "paused";
        case ThreadStatus::Canceled: return "canceled";
        case ThreadStatus::Completed: return "completed";
        case ThreadStatus::Failed: return "failed";
    }
    assert(false);
    return "unknown";
}

std::string LuaThread::status() const noexcept
{
    return threadStatusToString(pThreadData_->status);
}

sol::object LuaThread::getUserType(sol::state_view &lua) noexcept
{
    static sol::usertype<LuaThread> type(
        "new", sol::no_constructor,
        "cancel", &LuaThread::cancel,
        "pause", &LuaThread::pause,
        "resume", &LuaThread::resume,
        "status", &LuaThread::status,
        "wait", &LuaThread::wait
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

// class ThreadFactory

ThreadFactory::ThreadFactory(const sol::function& func) : stepwise_(false), step_(100U) {
    sol::state_view lua(func.lua_state());
    const sol::object& dumper = lua["string"]["dump"];
    ASSERT(dumper.valid() && dumper.get_type() == sol::type::function)
            << "Unable to get string.dump()";
    strFunction_ = static_cast<const sol::function&>(dumper)(func);

    // Inherit all pathes from parent state by default
    packagePath_ = lua["package"]["path"].get<std::string>();
    packageCPath_ = lua["package"]["cpath"].get<std::string>();
}

std::unique_ptr<LuaThread> ThreadFactory::runThread(const sol::variadic_args& args) {
    std::shared_ptr<LuaThread::ThreadData> threadData(new LuaThread::ThreadData);
    ASSERT(threadData.get());
    threadData->luaState.open_libraries(
        sol::lib::base, sol::lib::string,
        sol::lib::package, sol::lib::io, sol::lib::os
    );

    if (stepwise_)
        lua_sethook(threadData->luaState, LuaThread::luaHook, LUA_MASKCOUNT, step_);

    threadData->luaState["package"]["path"] = packagePath_;
    threadData->luaState["package"]["cpath"] = packageCPath_;

    // Inherit all pathes from parent state
    effil::LuaThread::getUserType(threadData->luaState);
    effil::ThreadFactory::getUserType(threadData->luaState);
    effil::SharedTable::getUserType(threadData->luaState);

    return std::make_unique<LuaThread>(threadData, strFunction_, args);
}

bool ThreadFactory::stepwise(const sol::optional<bool>& value)
{
    bool ret = stepwise_ ;
    if (value)
        stepwise_ = value.value();
    return ret;
}

unsigned int ThreadFactory::step(const sol::optional<unsigned int>& value)
{
    bool ret = step_;
    if (value)
        step_ = value.value();
    return ret;
}

std::string ThreadFactory::packagePath(const sol::optional<std::string>& value)
{
    std::string& ret = packagePath_;
    if (value)
        packagePath_ = value.value();
    return ret;
}

std::string ThreadFactory::packageCPath(const sol::optional<std::string>& value)
{
    std::string& ret = packageCPath_;
    if (value)
        packageCPath_ = value.value();
    return ret;
}

sol::object ThreadFactory::getUserType(sol::state_view &lua) noexcept
{
    static sol::usertype<ThreadFactory> type(
        "new", sol::no_constructor,
        sol::meta_function::call, &ThreadFactory::runThread,
        "stepwise", &ThreadFactory::stepwise,
        "step", &ThreadFactory::step,
        "package_path", &ThreadFactory::packagePath,
        "package_cpath", &ThreadFactory::packageCPath
    );
    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

} // effil
