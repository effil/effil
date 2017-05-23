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
    yield = capi.yield,
    size = capi.size,
    rawget = capi.rawget,
    rawset = capi.rawset,
    setmetatable = capi.setmetatable,
    getmetatable = capi.getmetatable,
    G = capi.G,
    gc = capi.gc,
    channel = capi.channel
}

api.type = function (something)
    local t = type(something)
    if (t ~= "userdata") then
        return t
    else
        return capi.userdata_type(something)
    end
end

local function run_thread(config, f, ...)
    return capi.thread(config.path, config.cpath, config.managed, config.step, f, ...)
end

-- Creates thread runner with given function
-- configurable parameters:
--     path - lua modules search path in child thread
--     cpath - lua libs search path in child thread
--     stepwise - is thread resumable
--     step - who fast reacte on state changing
--     __call - run thread, can be invoked multiple times
api.thread = function (f)
    local thread_config = {
        path = package.path,
        cpath = package.cpath,
        managed = true,
        step = 200 }
    setmetatable(thread_config, {__call = function(c, ...) return run_thread(c, f, ...) end})
    return thread_config
end

return api