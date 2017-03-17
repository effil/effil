#pragma once

#include <sol.hpp>
#include <thread>

namespace effil {

// Lua this thread API
std::string threadId();
void yield();
void sleep(int64_t, sol::optional<std::string>);

struct ThreadHandle;

class Thread {
public:
    Thread(const std::string& path,
           const std::string& cpath,
           bool stepwise,
           unsigned int step,
           const sol::function& function,
           const sol::variadic_args& args);
    ~Thread();
    static sol::object getUserType(sol::state_view& lua);

    std::tuple<sol::object, sol::table> get(sol::this_state state);
    std::string wait();
    void detach();
    bool joinable();
    void join();

    void cancel();
    void pause();
    void resume();
    std::string status() const;

private:
    std::shared_ptr<ThreadHandle> handle_;
    std::unique_ptr<std::thread> nativeThread_;

private:
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
};

} // effil
