#include "thread.h"

#include "thread-handle.h"
#include "stored-object.h"
#include "notifier.h"
#include "spin-mutex.h"
#include "utils.h"

#include <thread>
#include <sstream>

namespace effil {

using Status = ThreadHandle::Status;
using Command = ThreadHandle::Command;

namespace {

std::string statusToString(Status status) {
    switch (status) {
        case Status::Running:
            return "running";
        case Status::Paused:
            return "paused";
        case Status::Cancelled:
            return "cancelled";
        case Status::Completed:
            return "completed";
        case Status::Failed:
            return "failed";
    }
    assert(false);
    return "unknown";
}

int luaErrorHandler(lua_State* state) {
    luaL_traceback(state, state, nullptr, 1);
    const auto stacktrace = sol::stack::pop<std::string>(state);
    ThreadHandle::getThis()->result().emplace_back(createStoredObject(stacktrace));
    return 1;
}

const lua_CFunction luaErrorHandlerPtr = luaErrorHandler;

void luaHook(lua_State* L, lua_Debug*) {
    if (const auto thisThread = ThreadHandle::getThis()) {
        thisThread->performInterruptionPoint(L);
    }
}

} // namespace

void Thread::runThread(
    Thread thread,
    Function function,
    effil::StoredArray arguments)
{
    ThreadHandle::setThis(thread.ctx_.get());
    try {
        {
            ScopeGuard reportComplete([thread, &arguments](){
                // Let's destroy accociated state
                // to release all resources as soon as possible
                arguments.clear();
                thread.ctx_->destroyLua();
            });
            sol::protected_function userFuncObj = function.loadFunction(thread.ctx_->lua());

            #if LUA_VERSION_NUM > 501

            sol::stack::push(thread.ctx_->lua(), luaErrorHandlerPtr);
            userFuncObj.error_handler = sol::reference(thread.ctx_->lua());
            sol::stack::pop_n(thread.ctx_->lua(), 1);

            #endif // LUA_VERSION NUM > 501

            sol::protected_function_result result = userFuncObj(std::move(arguments));
            if (!result.valid()) {
                if (thread.ctx_->status() == Status::Cancelled)
                    return;

                sol::error err = result;
                std::string what = err.what();
                throw std::runtime_error(what);
            }

            sol::variadic_args args(thread.ctx_->lua(), -lua_gettop(thread.ctx_->lua()));
            for (const auto& iter : args) {
                StoredObject store = createStoredObject(iter.get<sol::object>());
                if (store->gcHandle() != nullptr)
                {
                    thread.ctx_->addReference(store->gcHandle());
                    store->releaseStrongReference();
                }
                thread.ctx_->result().emplace_back(std::move(store));
            }
        }
        thread.ctx_->changeStatus(Status::Completed);
    } catch (const std::exception& err) {
        if (thread.ctx_->command() == Command::Cancel && strcmp(err.what(), ThreadCancelException::message) == 0) {
            thread.ctx_->changeStatus(Status::Cancelled);
        } else {
            DEBUG("thread") << "Failed with msg: " << err.what() << std::endl;
            auto& returns = thread.ctx_->result();
            returns.insert(returns.begin(), {
                createStoredObject("failed"),
                createStoredObject(err.what())
            });
            thread.ctx_->changeStatus(Status::Failed);
        }
    }
}

void Thread::initialize(
    const std::string& path,
    const std::string& cpath,
    int step,
    const sol::function& function,
    const sol::variadic_args& variadicArgs)
{

    sol::optional<Function> functionObj;
    try {
        functionObj = GC::instance().create<Function>(function);
    } RETHROW_WITH_PREFIX("effil.thread");

    ctx_->lua()["package"]["path"] = path;
    ctx_->lua()["package"]["cpath"] = cpath;
    try {
        luaopen_effil(ctx_->lua());
        sol::stack::pop<sol::object>(ctx_->lua());
    } RETHROW_WITH_PREFIX("effil.thread");

    if (step != 0)
        lua_sethook(ctx_->lua(), luaHook, LUA_MASKCOUNT, step);

    effil::StoredArray arguments;
    try {
        for (const auto& arg : variadicArgs) {
            const auto& storedObj = createStoredObject(arg.get<sol::object>());
            ctx_->addReference(storedObj->gcHandle());
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

StoredArray Thread::status(const sol::this_state& lua) {
    const auto stat = ctx_->status();
    if (stat == Status::Failed) {
        assert(!ctx_->result().empty());
        return ctx_->result();
    } else {
        const sol::object luaStatus = sol::make_object(lua, statusToString(stat));
        return StoredArray({createStoredObject(luaStatus)});
    }
}

sol::optional<std::chrono::milliseconds> toOptionalTime(const sol::optional<int>& duration,
                                                        const sol::optional<std::string>& period) {
    if (duration)
        return fromLuaTime(*duration, period);
    else
        return sol::optional<std::chrono::milliseconds>();
}

StoredArray Thread::wait(const sol::this_state& lua,
                                                 const sol::optional<int>& duration,
                                                 const sol::optional<std::string>& period) {
    ctx_->waitForCompletion(toOptionalTime(duration, period));
    return status(lua);
}

StoredArray Thread::get(const sol::optional<int>& duration,
                       const sol::optional<std::string>& period) {
    if (ctx_->waitForCompletion(toOptionalTime(duration, period)) && ctx_->status() == Status::Completed)
        return ctx_->result();
    else
        return StoredArray();
}

bool Thread::cancel(const sol::this_state&,
                    const sol::optional<int>& duration,
                    const sol::optional<std::string>& period) {
    ctx_->putCommand(Command::Cancel);
    ctx_->interrupt();
    Status status = ctx_->waitForStatusChange(toOptionalTime(duration, period));
    return ThreadHandle::isFinishStatus(status);
}

bool Thread::pause(const sol::this_state&,
                   const sol::optional<int>& duration,
                   const sol::optional<std::string>& period) {
    ctx_->putCommand(Command::Pause);
    Status status = ctx_->waitForStatusChange(toOptionalTime(duration, period));
    return status == Status::Paused;
}

void Thread::resume() {
    ctx_->putCommand(Command::Run);
}

} // effil
