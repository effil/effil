#pragma once

#include <sol.hpp>

namespace effil {

struct IInterruptable;

namespace this_thread {

class ScopedSetInterruptable
{
public:
    ScopedSetInterruptable(IInterruptable* notifier);
    ~ScopedSetInterruptable();
};

void cancellationPoint();
std::string threadId();
void yield();
void sleep(const sol::stack_object& duration, const sol::stack_object& metric);
int pcall(lua_State* L);

} // namespace this_thread
} // namespace effil
