#pragma once

#include "lua-helpers.h"
#include "function.h"
#include "notifier.h"

#include <sol.hpp>

namespace effil {

// Lua this thread API
std::string threadId();
void yield();
void sleep(const sol::stack_object& duration, const sol::stack_object& metric);

class ThreadHandle : public GCData {
public:
    enum class Status {
        Running,
        Paused,
        Canceled,
        Completed,
        Failed
    };

    enum class Command {
        Run,
        Cancel,
        Pause
    };

public:
    ThreadHandle();
    Command command() const { return command_; }
    void putCommand(Command cmd);
    void changeStatus(Status stat);

    template <typename T>
    Status waitForStatusChange(const sol::optional<T>& time) {
        if (time)
            statusNotifier_.waitFor(*time);
        else
            statusNotifier_.wait();
        return status_;
    }

    template <typename T>
    Command waitForCommandChange(const sol::optional<T>& time) {
        if (time)
            commandNotifier_.waitFor(*time);
        else
            commandNotifier_.wait();
        return command_;
    }

    template <typename T>
    bool waitForCompletion(const sol::optional<T>& time) {
        if (time) {
            return completionNotifier_.waitFor(*time);
        }
        else {
            completionNotifier_.wait();
            return true;
        }
    }

    sol::state& lua() {
        assert(lua_);
        return  *lua_;
    }

    void destroyLua() { lua_.reset(); }

    Status status() { return status_; }

    StoredArray& result() { return result_; }

private:
    Status status_;
    Command command_;
    Notifier statusNotifier_;
    Notifier commandNotifier_;
    Notifier completionNotifier_;
    std::mutex stateLock_;
    StoredArray result_;

    std::unique_ptr<sol::state> lua_;
};

class Thread : public GCObject<ThreadHandle> {
public:
    static void exportAPI(sol::state_view& lua);

    StoredArray status(const sol::this_state& state);
    StoredArray wait(const sol::this_state& state,
                     const sol::optional<int>& duration,
                     const sol::optional<std::string>& period);
    StoredArray get(const sol::optional<int>& duration,
                   const sol::optional<std::string>& period);
    bool cancel(const sol::this_state& state,
                const sol::optional<int>& duration,
                const sol::optional<std::string>& period);
    bool pause(const sol::this_state&,
               const sol::optional<int>& duration,
               const sol::optional<std::string>& period);
    void resume();

private:
    Thread(const std::string& path,
           const std::string& cpath,
           int step,
           const sol::function& function,
           const sol::variadic_args& args);
    friend class GC;

private:
    static void runThread(Thread, Function, effil::StoredArray);
};

} // effil
