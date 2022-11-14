#include "thread-handle.h"

namespace effil {

// Thread specific pointer to current thread
static thread_local ThreadHandle* thisThreadHandle = nullptr;
static const sol::optional<std::chrono::milliseconds> NO_TIMEOUT;

ThreadHandle::ThreadHandle()
        : status_(Status::Running)
        , command_(Command::Run)
        , currNotifier_(nullptr)
        , lua_(std::make_unique<sol::state>())
{
    luaL_openlibs(*lua_);
}

void ThreadHandle::putCommand(Command cmd) {
    std::unique_lock<std::mutex> lock(stateLock_);
    if (isFinishStatus(status_) || command() == Command::Cancel)
        return;

    command_ = cmd;
    statusNotifier_.reset();
    commandNotifier_.notify();
}

void ThreadHandle::changeStatus(Status stat) {
    std::unique_lock<std::mutex> lock(stateLock_);
    status_ = stat;
    commandNotifier_.reset();
    statusNotifier_.notify();
    if (isFinishStatus(stat))
        completionNotifier_.notify();
}

void ThreadHandle::performInterruptionPoint() {
    switch (command()) {
        case Command::Run:
            break;
        case Command::Cancel:
            throw LuaHookStopException();
        case Command::Pause: {
            changeStatus(Status::Paused);
            Command cmd;
            do {
                cmd = waitForCommandChange(NO_TIMEOUT);
            } while(cmd != Command::Run && cmd != Command::Cancel);
            if (cmd == Command::Run) {
                changeStatus(Status::Running);
            } else {
                throw LuaHookStopException();
            }
            break;
        }
    }
}

ThreadHandle* ThreadHandle::getThis() {
    return thisThreadHandle;
}

void ThreadHandle::setThis(ThreadHandle* handle) {
    assert(handle);
    thisThreadHandle = handle;
}

} // namespace effil
