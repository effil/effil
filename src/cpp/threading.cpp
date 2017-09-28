#include "threading.h"

#include "stored-object.h"
#include "notifier.h"
#include "spin-mutex.h"
#include "utils.h"

#include <thread>
#include <sstream>

namespace effil {

namespace {

// Doesn't inherit std::exception
// to prevent from catching this exception third party lua C++ libs
class LuaHookStopException {};

enum class Status {
    Running,
    Paused,
    Canceled,
    Completed,
    Failed
};

bool isFinishStatus(Status stat) {
    return stat == Status::Canceled || stat == Status::Completed || stat == Status::Failed;
}

std::string statusToString(Status status) {
    switch (status) {
        case Status::Running:
            return "running";
        case Status::Paused:
            return "paused";
        case Status::Canceled:
            return "canceled";
        case Status::Completed:
            return "completed";
        case Status::Failed:
            return "failed";
    }
    assert(false);
    return "unknown";
}

enum class Command {
    Run,
    Cancel,
    Pause
};

} // namespace

class ThreadHandle {
public:
    ThreadHandle()
            : status_(Status::Running)
            , command_(Command::Run)
            , lua_(std::make_unique<sol::state>()) {
        luaL_openlibs(*lua_);
    }

    Command command() const { return command_; }

    void putCommand(Command cmd) {
        std::unique_lock<std::mutex> lock(stateLock_);
        if (isFinishStatus(status_))
            return;

        command_ = cmd;
        statusNotifier_.reset();
        commandNotifier_.notify();
    }

    void changeStatus(Status stat) {
        std::unique_lock<std::mutex> lock(stateLock_);
        status_ = stat;
        commandNotifier_.reset();
        statusNotifier_.notify();
        if (isFinishStatus(stat))
            completionNotifier_.notify();
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

namespace  {

const sol::optional<std::chrono::milliseconds> NO_TIMEOUT;

static thread_local ThreadHandle* thisThreadHandle = nullptr;

void luaHook(lua_State*, lua_Debug*) {
    assert(thisThreadHandle);
    switch (thisThreadHandle->command()) {
        case Command::Run:
            break;
        case Command::Cancel:
            throw LuaHookStopException();
        case Command::Pause: {
            thisThreadHandle->changeStatus(Status::Paused);
            Command cmd;
            do {
                cmd = thisThreadHandle->waitForCommandChange(NO_TIMEOUT);
            } while(cmd != Command::Run && cmd != Command::Cancel);
            if (cmd == Command::Run)
                thisThreadHandle->changeStatus(Status::Running);
            else
                throw LuaHookStopException();
            break;
        }
    }
}

void runThread(std::shared_ptr<ThreadHandle> handle,
               std::string strFunction,
               effil::StoredArray arguments) {
    assert(handle);
    thisThreadHandle = handle.get();

    try {
        {
            ScopeGuard reportComplete([handle, &arguments](){
                DEBUG << "Finished " << std::endl;
                // Let's destroy accociated state
                // to release all resources as soon as possible
                arguments.clear();
                handle->destroyLua();
            });
            sol::function userFuncObj = loadString(handle->lua(), strFunction);
            sol::function_result results = userFuncObj(std::move(arguments));
            (void)results; // just leave all returns on the stack
            sol::variadic_args args(handle->lua(), -lua_gettop(handle->lua()));
            for (const auto& iter : args) {
                StoredObject store = createStoredObject(iter.get<sol::object>());
                handle->result().emplace_back(std::move(store));
            }
        }
        handle->changeStatus(Status::Completed);
    } catch (const LuaHookStopException&) {
        handle->changeStatus(Status::Canceled);
    } catch (const sol::error& err) {
        DEBUG << "Failed with msg: " << err.what() << std::endl;
        handle->result().emplace_back(createStoredObject(err.what()));
        handle->changeStatus(Status::Failed);
    }
}

} // namespace


std::string threadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void yield() {
    if (thisThreadHandle)
        luaHook(nullptr, nullptr);
    std::this_thread::yield();
}

void sleep(const sol::stack_object& duration, const sol::stack_object& metric) {
    if (duration.valid()) {
        REQUIRE(duration.get_type() == sol::type::number) << "bad argument #1 to 'effil.sleep' (number expected, got " << luaTypename(duration) << ")";
        if (metric.valid())
            REQUIRE(metric.get_type() == sol::type::string) << "bad argument #2 to 'effil.sleep' (string expected, got " << luaTypename(metric) << ")";
        try {
            std::this_thread::sleep_for(fromLuaTime(duration.as<int>(), metric.valid() ? metric.as<std::string>() : sol::optional<std::string>()));
        } RETHROW_WITH_PREFIX("effil.sleep");
    }
    else {
        yield();
    }
}

Thread::Thread(const std::string& path,
       const std::string& cpath,
       int step,
       const sol::function& function,
       const sol::variadic_args& variadicArgs)
        : handle_(std::make_shared<ThreadHandle>()) {

    handle_->lua()["package"]["path"] = path;
    handle_->lua()["package"]["cpath"] = cpath;
    handle_->lua().script("require 'effil'");

    if (step != 0)
        lua_sethook(handle_->lua(), luaHook, LUA_MASKCOUNT, step);

    std::string strFunction = dumpFunction(function);

    effil::StoredArray arguments;
    try {
        for (const auto& arg : variadicArgs) {
            arguments.emplace_back(createStoredObject(arg.get<sol::object>()));
        }
    } RETHROW_WITH_PREFIX("effil.thread");

    std::thread thr(&runThread,
                    handle_,
                    std::move(strFunction),
                    std::move(arguments));
    DEBUG << "Created " << thr.get_id() << std::endl;
    thr.detach();
}

void Thread::exportAPI(sol::state_view& lua) {
    sol::usertype<Thread> type(
            "new", sol::no_constructor,
            "get", &Thread::get,
            "wait", &Thread::wait,
            "cancel", &Thread::cancel,
            "pause", &Thread::pause,
            "resume", &Thread::resume,
            "status", &Thread::status);

    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

std::pair<sol::object, sol::object> Thread::status(const sol::this_state& lua) {
    sol::object luaStatus = sol::make_object(lua, statusToString(handle_->status()));

    if (handle_->status() == Status::Failed) {
        assert(!handle_->result().empty());
        return std::make_pair(luaStatus, handle_->result()[0]->unpack(lua));
    } else {
        return std::make_pair(luaStatus, sol::nil);
    }
}

sol::optional<std::chrono::milliseconds> toOptionalTime(const sol::optional<int>& duration,
                                                        const sol::optional<std::string>& period) {
    if (duration)
        return fromLuaTime(*duration, period);
    else
        return sol::optional<std::chrono::milliseconds>();
}

std::pair<sol::object, sol::object> Thread::wait(const sol::this_state& lua,
                                                 const sol::optional<int>& duration,
                                                 const sol::optional<std::string>& period) {
    handle_->waitForCompletion(toOptionalTime(duration, period));
    return status(lua);
}

StoredArray Thread::get(const sol::optional<int>& duration,
                       const sol::optional<std::string>& period) {
    if (handle_->waitForCompletion(toOptionalTime(duration, period)) && handle_->status() == Status::Completed)
        return handle_->result();
    else
        return StoredArray();
}

bool Thread::cancel(const sol::this_state&,
                    const sol::optional<int>& duration,
                    const sol::optional<std::string>& period) {
    handle_->putCommand(Command::Cancel);
    Status status = handle_->waitForStatusChange(toOptionalTime(duration, period));
    return isFinishStatus(status);
}

bool Thread::pause(const sol::this_state&,
                   const sol::optional<int>& duration,
                   const sol::optional<std::string>& period) {
    handle_->putCommand(Command::Pause);
    Status status = handle_->waitForStatusChange(toOptionalTime(duration, period));
    return status == Status::Paused;
}

void Thread::resume() {
    handle_->putCommand(Command::Run);
}

} // effil
