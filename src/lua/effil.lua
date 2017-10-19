local capi = require 'libeffil'

local api = {
    version = "0.1.0",
    table = capi.table,
    thread_id = capi.thread_id,
    sleep = capi.sleep,
    yield = capi.yield,
    rawget = capi.rawget,
    rawset = capi.rawset,
    setmetatable = capi.setmetatable,
    getmetatable = capi.getmetatable,
    G = capi.G,
    gc = capi.gc,
    channel = capi.channel,
    type = capi.type,
    pairs = capi.pairs,
    ipairs = capi.ipairs,
    allow_table_upvalues = capi.allow_table_upvalues
}

api.size = function (something)
    local t = api.type(something)
    if t == "effil.table" then
        return capi.table_size(something)
    elseif t == "effil.channel" then
        return something:size()
    else
        error("Unsupported type " .. t .. " for effil.size()")
    end
end

local function run_thread(config, f, ...)
    return capi.thread(config.path, config.cpath, config.step, f, ...)
end

-- Creates thread runner with given function
-- configurable parameters:
--     path - lua modules search path in child thread
--     cpath - lua libs search path in child thread
--     step - who fast reacte on state changing
--     __call - run thread, can be invoked multiple times
api.thread = function (f)
    if type(f) ~= "function" then
        error("bad argument #1 to 'effil.thread' (function expected, got " .. capi.type(f) .. ")")
    end

    local thread_config = {
        path = package.path,
        cpath = package.cpath,
        step = 200 }
    setmetatable(thread_config, {__call = function(c, ...) return run_thread(c, f, ...) end})
    return thread_config
end

return api