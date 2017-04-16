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
    const bool managed;

    Status status;
    StoredArray result;

    Notifier completion;
    // on thread resume
    Notifier pause;
    // used only with sync pause
    Notifier syncPause;

public:
    ThreadHandle(bool isManaged)
            : managed(isManaged)
            , status(Status::Running)
            , command_(Command::Run)
            , lua_(std::make_unique<sol::state>()) {
        luaL_openlibs(*lua_);
    }

    Command command() const { return command_; }

    void command(Command cmd) {
        std::lock_guard<SpinMutex> lock(commandMutex_);
        if (command_ == Command::Cancel)
            return;
        command_ = cmd;
    }

    sol::state& lua() {
        assert(lua_);
        return  *lua_;
    }

    void destroyLua() { lua_.reset(); }

private:
    SpinMutex commandMutex_;
    Command command_;

    std::unique_ptr<sol::state> lua_;
};

namespace  {

static thread_local ThreadHandle* thisThreadHandle = nullptr;

void luaHook(lua_State*, lua_Debug*) {
    assert(thisThreadHandle);
    switch (thisThreadHandle->command()) {
        case Command::Run:
            break;
        case Command::Cancel:
            throw LuaHookStopException();
        case Command::Pause: {
            thisThreadHandle->status = Status::Paused;
            thisThreadHandle->syncPause.notify();
            thisThreadHandle->pause.wait();
            if (thisThreadHandle->command() == Command::Run)
                thisThreadHandle->status = Status::Running;
            else
                throw LuaHookStopException();
            break;
        }
    }
}

class ScopeGuard {
public:
    ScopeGuard(const std::function<void()>& f) : f_(f) {}
    ~ScopeGuard() { f_(); }

private:
    std::function<void()> f_;
};

void runThread(std::shared_ptr<ThreadHandle> handle,
               std::string strFunction,
               effil::StoredArray arguments) {

    ScopeGuard reportComplete([=](){
        DEBUG << "Finished " << std::endl;
        // Let's destroy accociated state
        // to release all resources as soon as possible
        handle->destroyLua();
        handle->completion.notify();
    });

    assert(handle);
    thisThreadHandle = handle.get();

    try {
        sol::function userFuncObj = loadString(handle->lua(), strFunction);
        sol::function_result results = userFuncObj(std::move(arguments));
        (void)results; // just leave all returns on the stack
        sol::variadic_args args(handle->lua(), -lua_gettop(handle->lua()));
        for (const auto& iter : args) {
            StoredObject store = createStoredObject(iter.get<sol::object>());
            handle->result.emplace_back(std::move(store));
        }
        handle->status = Status::Completed;
    } catch (const LuaHookStopException&) {
        handle->status = Status::Canceled;
    } catch (const sol::error& err) {
        DEBUG << "Failed with msg: " << err.what() << std::endl;
        handle->result.emplace_back(createStoredObject(err.what()));
        handle->status = Status::Failed;
    }
}

} // namespace


std::string threadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void yield() { std::this_thread::yield(); }

void sleep(const sol::optional<int>& duration, const sol::optional<std::string>& period) {
    if (duration)
        std::this_thread::sleep_for(fromLuaTime(*duration, period));
    else
        yield();
}

Thread::Thread(const std::string& path,
       const std::string& cpath,
       bool managed,
       unsigned int step,
       const sol::function& function,
       const sol::variadic_args& variadicArgs)
        : handle_(std::make_shared<ThreadHandle>(managed)) {

    handle_->lua()["package"]["path"] = path;
    handle_->lua()["package"]["cpath"] = cpath;
    handle_->lua().script("require 'effil'");

    if (managed)
        lua_sethook(handle_->lua(), luaHook, LUA_MASKCOUNT, step);

    std::string strFunction = dumpFunction(function);

    effil::StoredArray arguments;
    for (const auto& arg : variadicArgs) {
        arguments.emplace_back(createStoredObject(arg.get<sol::object>()));
    }

    std::thread thr(&runThread,
                    handle_,
                    std::move(strFunction),
                    std::move(arguments));
    DEBUG << "Created " << thr.get_id() << std::endl;
    thr.detach();
}

void Thread::getUserType(sol::state_view& lua) {
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
    sol::object luaStatus = sol::make_object(lua, statusToString(handle_->status));

    if (handle_->status == Status::Failed) {
        assert(!handle_->result.empty());
        return std::make_pair(luaStatus, handle_->result[0]->unpack(lua));
    } else {
        return std::make_pair(luaStatus, sol::nil);
    }
}

bool Thread::waitFor(const sol::optional<int>& duration,
             const sol::optional<std::string>& period) {
    if (!duration) { // sync version
        handle_->completion.wait();
        return true;
    } else { // async version
        return handle_->completion.waitFor(fromLuaTime(*duration, period));
    }
}

std::pair<sol::object, sol::object> Thread::wait(const sol::this_state& lua,
                                                 const sol::optional<int>& duration,
                                                 const sol::optional<std::string>& period) {
    waitFor(duration, period);
    return status(lua);
}

StoredArray Thread::get(const sol::optional<int>& duration,
                       const sol::optional<std::string>& period) {
    bool completed = waitFor(duration, period);
    if (completed && handle_->status == Status::Completed)
        return handle_->result;
    else
        return StoredArray();
}

bool Thread::cancel(const sol::this_state&,
                    const sol::optional<int>& duration,
                    const sol::optional<std::string>& period) {
    REQUIRE(handle_->managed) << "Unable to cancel: unmanaged thread";

    handle_->command(Command::Cancel);
    handle_->pause.notify();

    if (handle_->status == Status::Running) {
        return waitFor(duration, period);
    } else {
        handle_->completion.wait();
        return true;
    }
}

bool Thread::pause(const sol::this_state&,
                   const sol::optional<int>& duration,
                   const sol::optional<std::string>& period) {
    REQUIRE(handle_->managed) << "Unable to pause: unmanaged thread";

    handle_->pause.reset();
    handle_->command(Command::Pause);

    if (!duration) { // sync wait
        handle_->syncPause.wait();
        return true;
    } else { // async wait
        return handle_->syncPause.waitFor(fromLuaTime(*duration, period));
    }
}

void Thread::resume() {
    REQUIRE(handle_->managed) <<  "Unable to resume: unmanaged thread";

    handle_->command(Command::Run);
    handle_->syncPause.reset();
    handle_->pause.notify();
}

} // effil
