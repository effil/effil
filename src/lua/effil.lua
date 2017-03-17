local function detect_native_lib_ext()
    local home = os.getenv("HOME")
    if not home then return "dll" end
    if string.find(home, "/Users/") then return "dylib" end
    if string.find(home, "/home/") then return "so" end
    -- TODO: unable to detect os
    -- Unix, is it you?
    return "so"
end

package.cpath = package.cpath .. ";./?." .. detect_native_lib_ext()

local capi = require 'libeffil'
local api = {
    version = "0.1.0",
    table = capi.table,
    thread_id = capi.thread_id,
    sleep = capi.sleep,
    yield = capi.yield
}

local function run_thread(config, f, ...)
    local path = config.path or package.path
    local cpath = config.cpath or package.cpath
    local stepwise = config.stepwise or false
    local step = config.step or 200

    return capi.thread(path, cpath, stepwise, step, f, ...)
end

-- Creates thread runner with given function
-- configurable parameters:
--     path - lua modules search path in child thread
--     cpath - lua libs search path in child thread
--     stepwise - is thread resumable
--     step - who fast reacte on state changing
--     __call - run thread, can be invoked multiple times
api.thread = function (f)
    local thread_config = {}
    setmetatable(thread_config, {__call = function(c, ...) return run_thread(c, f, ...) end})
    return thread_config
end

-- Experimantal API
-- Desined to be simple is possible (minifuture)
-- Usage:
--    effil.async(f, arg1, arg2):get() -> returns result ans status
--    effil.async(f, arg1):wait() -> just perform operation and get status
api.async = function (f, ...)
    local thread = run_thread({}, f, ...)
    return {
        get = function()
            local status, result = thread:get()
            if (status ~= "completed") then
                return nil
            end
            return result[1]
        end,
        wait = function ()
            thread:wait()
        end
    }
end

return api