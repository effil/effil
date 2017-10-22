#include "threading.h"

#include "stored-object.h"
#include "spin-mutex.h"
#include "utils.h"

#include <thread>
#include <sstream>

namespace effil {

using Status = ThreadImpl::Status;
using Command = ThreadImpl::Command;

namespace {

// Doesn't inherit std::exception
// to prevent from catching this exception third party lua C++ libs
class LuaHookStopException {};

std::string statusToString(ThreadImpl::Status status) {
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

} // namespace

namespace  {

const sol::optional<std::chrono::milliseconds> NO_TIMEOUT;

static thread_local ThreadImpl* thisThreadHandle = nullptr;

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

} // namespace

void Thread::runThread(Thread thread,
               Function function,
               effil::StoredArray arguments) {
    thisThreadHandle = thread.impl_.get();
    assert(thisThreadHandle != nullptr);

    try {
        {
            ScopeGuard reportComplete([thread, &arguments](){
                // Let's destroy associated state
                // to release all resources as soon as possible
                arguments.clear();
                thread.impl_->destroyLua();
            });
            sol::function userFuncObj = function.loadFunction(thread.impl_->lua());
            sol::function_result results = userFuncObj(std::move(arguments));
            (void)results; // just leave all returns on the stack
            sol::variadic_args args(thread.impl_->lua(), -lua_gettop(thread.impl_->lua()));
            for (const auto& iter : args) {
                StoredObject store = createStoredObject(iter.get<sol::object>());
                if (store->gcHandle() != nullptr) {
                    thread.impl_->addReference(store->gcHandle());
                    store->releaseStrongReference();
                }
                thread.impl_->result().emplace_back(std::move(store));
            }
        }
        thread.impl_->changeStatus(Status::Completed);
    } catch (const LuaHookStopException&) {
        thread.impl_->changeStatus(Status::Canceled);
    } catch (const sol::error& err) {
        DEBUG << "Failed with msg: " << err.what() << std::endl;
        thread.impl_->result().emplace_back(createStoredObject(err.what()));
        thread.impl_->changeStatus(Status::Failed);
    }
}

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
       const sol::variadic_args& variadicArgs) {

    sol::optional<Function> functionObj;
    try {
        functionObj = GC::instance().create<Function>(function);
    } RETHROW_WITH_PREFIX("effil.thread");

    impl_->lua()["package"]["path"] = path;
    impl_->lua()["package"]["cpath"] = cpath;
    impl_->lua().script("require 'effil'");

    if (step != 0)
        lua_sethook(impl_->lua(), luaHook, LUA_MASKCOUNT, step);

    effil::StoredArray arguments;
    try {
        for (const auto& arg : variadicArgs) {
            const auto& storedObj = createStoredObject(arg.get<sol::object>());
            impl_->addReference(storedObj->gcHandle());
            storedObj->releaseStrongReference();
            arguments.emplace_back(storedObj);
        }
    } RETHROW_WITH_PREFIX("effil.thread");

    std::thread thr(&Thread::runThread,
                    *this,
                    functionObj.value(),
                    std::move(arguments));
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
    sol::object luaStatus = sol::make_object(lua, statusToString(impl_->status()));

    if (impl_->status() == Status::Failed) {
        assert(!impl_->result().empty());
        return std::make_pair(luaStatus, impl_->result()[0]->unpack(lua));
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
    impl_->waitForCompletion(toOptionalTime(duration, period));
    return status(lua);
}

StoredArray Thread::get(const sol::optional<int>& duration,
                       const sol::optional<std::string>& period) {
    if (impl_->waitForCompletion(toOptionalTime(duration, period)) && impl_->status() == Status::Completed)
        return impl_->result();
    else
        return StoredArray();
}

bool Thread::cancel(const sol::this_state&,
                    const sol::optional<int>& duration,
                    const sol::optional<std::string>& period) {
    impl_->putCommand(Command::Cancel);
    Status status = impl_->waitForStatusChange(toOptionalTime(duration, period));
    return ThreadImpl::isFinishStatus(status);
}

bool Thread::pause(const sol::this_state&,
                   const sol::optional<int>& duration,
                   const sol::optional<std::string>& period) {
    impl_->putCommand(Command::Pause);
    Status status = impl_->waitForStatusChange(toOptionalTime(duration, period));
    return status == Status::Paused;
}

void Thread::resume() {
    impl_->putCommand(Command::Run);
}

} // effil
