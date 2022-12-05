#pragma once

#include "lua-helpers.h"
#include "notifier.h"
#include "gc-data.h"

#include <sol.hpp>

namespace effil {

class ThreadCancelException : public std::runtime_error
{
public:
    static constexpr auto message = "Effil: thread is cancelled";

    ThreadCancelException()
        : std::runtime_error(message)
    {}
};

class Thread;

class ThreadHandle : public GCData {
public:
    enum class Status {
        Running,
        Paused,
        Cancelled,
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
    void performInterruptionPoint(lua_State* L);
    void performInterruptionPointThrow();

    static ThreadHandle* getThis();

    static bool isFinishStatus(Status stat) {
        return stat == Status::Cancelled || stat == Status::Completed || stat == Status::Failed;
    }

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

    Status status() const { return status_; }

    StoredArray& result() { return result_; }

    void setNotifier(IInterruptable* notifier) {
        currNotifier_ = notifier;
    }

    void interrupt() const {
        IInterruptable* currNotifier = currNotifier_;
        if (currNotifier)
            currNotifier->interrupt();
    }

private:
    Status status_;
    Command command_;
    Notifier statusNotifier_;
    Notifier commandNotifier_;
    Notifier completionNotifier_;
    std::mutex stateLock_;
    StoredArray result_;
    IInterruptable* currNotifier_;
    std::unique_ptr<sol::state> lua_;

    void performInterruptionPointImpl(const std::function<void(void)>& cancelClbk);

    static void setThis(ThreadHandle* handle);
    friend class Thread;
};

} // namespace effil
