#include "threading.h"

#include "shared-table.h"
#include "stored-object.h"
#include "utils.h"

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
    Failed,
    Detached,
};

enum class ThreadCommand {
    Nothing,
    Cancel,
    Pause,
    Resume,
};

} // namespace

struct ThreadHandle {
    sol::state luaState;
    std::atomic<Status> status;
    std::atomic<ThreadCommand> command;
    std::vector<StoredObject> results;

    bool isThreadAlive() const {
        return status == Status::Running || status == Status::Paused;
    }
};

namespace  {

static thread_local ThreadHandle* thisThreadHandle = nullptr;

void luaHook(lua_State*, lua_Debug*) {
    if (thisThreadHandle) {
        switch (thisThreadHandle->command) {
            case ThreadCommand::Pause: {
                thisThreadHandle->status = Status::Paused;
                ThreadCommand cmd = thisThreadHandle->command;
                while (cmd == ThreadCommand::Pause) {
                    std::this_thread::yield();
                    cmd = thisThreadHandle->command;
                }
                assert(cmd != ThreadCommand::Nothing);
                if (cmd == ThreadCommand::Resume) {
                    thisThreadHandle->status = Status::Running;
                    break; // Just go out of the function
                } else {   /* HOOK_STOP - do nothing and go to the next case */
                }
            }
            case ThreadCommand::Cancel:
                throw LuaHookStopException();
            default:
            case ThreadCommand::Nothing:
                break;
        }
    }
}

void runThread(std::shared_ptr<ThreadHandle> handle, const std::string &strFunction,
               std::vector<sol::object> &&arguments) {
    try {
        thisThreadHandle = handle.get();
        const sol::object &stringLoader = handle->luaState["loadstring"];
        REQUIRE(stringLoader.valid() && stringLoader.get_type() == sol::type::function)
                    << "Invalid loadstring function";
        sol::function userFuncObj = static_cast<const sol::function &>(stringLoader)(strFunction);
        sol::function_result results = userFuncObj(sol::as_args(arguments));
        (void) results; // TODO: try to avoid use of useless sol::function_result here
        sol::variadic_args args(handle->luaState, -lua_gettop(handle->luaState));
        for (const auto &iter : args) {
            StoredObject store = createStoredObject(iter.get<sol::object>());
            handle->results.emplace_back(std::move(store));
        }
        handle->status = Status::Completed;
    } catch (const LuaHookStopException &) {
        handle->status = Status::Canceled;
    } catch (const sol::error &err) {
        handle->status = Status::Failed;
        sol::stack::push(handle->luaState, err.what());
        StoredObject store = createStoredObject(sol::stack::pop<sol::object>(handle->luaState));
        handle->results.emplace_back(std::move(store));
    }
}

} // namespace


std::string threadId() {
    return (std::stringstream() << std::this_thread::get_id()).str();
}

void yield() { std::this_thread::yield(); }

void sleep(int64_t time, sol::optional<std::string> period) {
    std::string metric = period ? period.value() : "s";
    if (metric == "ms")
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    else if (metric == "s")
        std::this_thread::sleep_for(std::chrono::seconds(time));
    else if (metric == "m")
        std::this_thread::sleep_for(std::chrono::minutes(time));
    else
        throw sol::error("invalid time identification: " + metric);
}

Thread::Thread(const std::string& path,
       const std::string& cpath,
       bool stepwise,
       unsigned int step,
       const sol::function& function,
       const sol::variadic_args& variadicArgs)
        : handle_(std::make_shared<ThreadHandle>()) {
    openAllLibs(handle_->luaState);
    handle_->command = ThreadCommand::Nothing;
    handle_->status = Status::Running;
    handle_->luaState["package"]["path"] = path;
    handle_->luaState["package"]["cpath"] = cpath;
    handle_->luaState.script("require 'effil'");

    if (stepwise)
        lua_sethook(handle_->luaState, luaHook, LUA_MASKCOUNT, step);

    sol::state_view lua(function.lua_state());
    const sol::object& dumper = lua["string"]["dump"];
    std::string strFunction = static_cast<const sol::function&>(dumper)(function);

    std::vector<sol::object> arguments;
    for (const auto& arg : variadicArgs) {
        StoredObject store = createStoredObject(arg.get<sol::object>());
        arguments.push_back(store->unpack(sol::this_state{handle_->luaState}));
    }
    nativeThread_.reset(new std::thread(&runThread, handle_, strFunction, std::move(arguments)));
    DEBUG << "Started thread " << nativeThread_->get_id() << std::endl;
}

Thread::~Thread() {
    // FIXME: may be assert? or join?
    REQUIRE(!nativeThread_->joinable())
         << "Delete not detached thread " << nativeThread_->get_id();
}

sol::object Thread::getUserType(sol::state_view& lua) {
    static sol::usertype<Thread> type(
            "new", sol::no_constructor,
            "get", &Thread::get,
            "wait", &Thread::wait,
            "join", &Thread::join,
            "detach", &Thread::detach,
            "joinable", &Thread::joinable,
            "cancel", &Thread::cancel,
            "pause", &Thread::pause,
            "resume", &Thread::resume,
            "status", &Thread::status);

    sol::stack::push(lua, type);
    return sol::stack::pop<sol::object>(lua);
}

std::tuple<sol::object, sol::table> Thread::get(sol::this_state state) {
    REQUIRE(nativeThread_->joinable()) << "Thread " << nativeThread_->get_id() << " not joinable";
    nativeThread_->join();

    sol::table returns = sol::state_view(state).create_table();

    if (handle_->status == Status::Completed)
        for (const StoredObject& obj : handle_->results)
            returns.add(obj->unpack(state));

    return std::make_tuple(sol::make_object(state, status()), std::move(returns));
}

std::string Thread::wait() {
    DEBUG << "Wait " << nativeThread_->get_id() << std::endl;
    nativeThread_->join();
    return status();
}

void Thread::detach()
{
    REQUIRE(handle_->status == Status::Running);
    handle_->status = Status::Detached;
    nativeThread_->detach();
}

bool Thread::joinable() { return nativeThread_->joinable(); };

void Thread::join() {
    DEBUG << "Join " << nativeThread_->get_id() << std::endl;
    nativeThread_->join();
}

void Thread::cancel() {
    REQUIRE(handle_->isThreadAlive())
        << "Thread " <<  nativeThread_->get_id() << " already canceled";

    DEBUG << "Cancel " << nativeThread_->get_id() << std::endl;
    handle_->command = ThreadCommand::Cancel;
    nativeThread_->join();
}

void Thread::pause() {
    REQUIRE(handle_->isThreadAlive())
                << "Unable not pause thread " << nativeThread_->get_id();
    handle_->command = ThreadCommand::Pause;
}

void Thread::resume() {
    REQUIRE(handle_->isThreadAlive())
        << "Unable not resume thread " << nativeThread_->get_id();
    handle_->command = ThreadCommand::Resume;
}

std::string Thread::status() const {
    switch (handle_->status) {
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
        case Status::Detached:
            return "detached";
    }
    assert(false);
    return "unknown";
}


} // effil
