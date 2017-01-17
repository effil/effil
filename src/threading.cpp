extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <sol.hpp>
#include <iostream>
#include <sstream>
#include <thread>

#include "shared-table.h"

core::SharedTable shareTable;

class LuaThread
{
public:

    LuaThread(const std::string& rawFunction)
        : m_strFunction(rawFunction)
    {
        std::cout << "LuaThread" << std::endl;
        m_pState.reset(new sol::state);
        core::SharedTable::bind(*m_pState);
        (*m_pState)["share"] = &shareTable;
        assert(m_pState.get() != NULL);
        m_pState->open_libraries(
            sol::lib::base, sol::lib::string,
            sol::lib::package, sol::lib::io, sol::lib::os
        );
        m_pThread.reset(new std::thread(&LuaThread::Impl, this));
        assert(m_pThread.get() != NULL);
        std::cout << "LuaThread##" << std::endl;
    }

    virtual ~LuaThread()
    {
        std::cout << "~LuaThread" << std::endl;
        Join();
    }

    void Join()
    {
        std::cout << "Join started" << std::endl;
        if (m_pThread.get())
        {
            m_pThread->join();
            m_pThread.reset();
        }
        if (m_pState.get())
        {
            m_pState.reset();
        }
        std::cout << "Join finished" << std::endl;
    }

private:
    void Impl()
    {
        std::cout << "Impl" << std::endl;
        std::stringstream script;
        script << "loadstring(" << m_strFunction << ")()";
        m_pState->script(script.str());
    }

    std::string m_strFunction;
    std::shared_ptr<sol::state> m_pState;
    std::shared_ptr<std::thread> m_pThread;
};

static std::string ThreadId()
{
    std::stringstream ss;
    ss << std::hash<std::thread::id>()(std::this_thread::get_id());
    return ss.str();
}

extern "C" int luaopen_libbevy(lua_State *L)
{
    sol::state_view lua(L);
    lua.new_usertype<LuaThread>("thread",
        sol::constructors<sol::types<std::string>>(),
        "join", &LuaThread::Join
    );
    lua["thread"]["thread_id"] = ThreadId;

    core::SharedTable::bind(lua);
    lua["share"] = &shareTable;
    return 0;
}
