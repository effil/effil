#pragma once

#include "shared-table.h"

#include <iostream>
#include <sstream>
#include <thread>

namespace effil {

// Lua this thread API
std::string threadId() noexcept;
void yield() noexcept;
void sleep(int64_t, sol::optional<std::string>) noexcept;

class LuaThread {
public:
    enum class ThreadStatus {
        Running,
        Paused,
        Canceled,
        Completed,
        Failed,
    };

    enum class ThreadCommand
    {
        Nothing = 1,
        Cancel,
        Pause,
        Resume,
    };

    struct ThreadData{
        sol::state luaState;
        std::atomic<ThreadStatus> status;
        std::atomic<ThreadCommand> command;
        std::vector<StoredObject> results;
    };

    LuaThread(std::shared_ptr<ThreadData> threadData, const std::string& function, const sol::variadic_args& args);
    static sol::object getUserType(sol::state_view &lua) noexcept;
    static void luaHook(lua_State*, lua_Debug*);

    /* Public lua methods*/
    void cancel() noexcept;
    void pause() noexcept;
    void resume() noexcept;
    std::string status() const noexcept;
    std::tuple<sol::object, sol::table> wait(sol::this_state state) const noexcept;

private:
    LuaThread(const LuaThread&) = delete;
    LuaThread& operator=(const LuaThread&) = delete;

    std::string threadStatusToString(ThreadStatus stat) const noexcept;
    static void work(std::shared_ptr<ThreadData> threadData, const std::string strFunction, std::vector<sol::object>&& arguments) noexcept;

    std::shared_ptr<ThreadData> pThreadData_;
    std::shared_ptr<std::thread> pThread_;

    static thread_local LuaThread::ThreadData* pThreadLocalData;
};

class ThreadFactory {
public:
    ThreadFactory(const sol::function& func);
    static sol::object getUserType(sol::state_view &lua) noexcept;

    /* Public lua methods*/
    std::unique_ptr<LuaThread> runThread(const sol::variadic_args& args);
    bool stepwise(const sol::optional<bool>&);
    unsigned int step(const sol::optional<unsigned int>&);
    std::string packagePath(const sol::optional<std::string>&);
    std::string packageCPath(const sol::optional<std::string>&);

private:
    std::string strFunction_;
    bool stepwise_;
    unsigned int step_;
    std::string packagePath_;
    std::string packageCPath_;
};

} // effil
