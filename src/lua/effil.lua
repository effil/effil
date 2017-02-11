-- This is just a blueprint of API
-- all API have to be 100% thread safe
local effil = require("effil")

-- Common API --
-- idetify type of varible
-- it can be lua types or etable/ethread/mutex.
local t = effil.type(someVar)

-- Shared tables --
local st = effil.table() -- create new table
-- pairs/ipairs like regutal tables
for k, v in pairs(st) do end
for i, v in ipairs do end

-- may be not
local keys = effil.table.keys(st)

-- Garbage collector --
effil.gc.collectgarbage() -- like lua collectgarbage
effil.gc.step = 1000 -- set gc step
print(effil.gc.step) -- get step value
effil.gc.stop()
effil.gc.resume()
print(effil.gc.count) -- get number of manage objects

-- Threads --
local counter = effil.thread(function()
    for i, v in {"one", "two", "three"} do
        yeild(v)
    end
end)

for i, v in counter.get() do
    print(v) -- one twp three
end

local sthread = effil.thread(effil.thread.stepwise, function (st)
    for i, v in pairs(st) do
        print(i, v)
    end
end, st)
sthread.wait()

-- Alternative --
local sthread = effil.thread.stepwise(function (st) end)


-- Mutex --
-- Nothing spesial here
local mutex = effil.mutex()
local smutex = effil.mutex(effil.mutex.spin)
local rmutex = effil.mutex(effil.mutex.recursive)
local shmutex = effil.mutex(effil.mutex.shared)
mutex.lock()
mutex.unlock()
mutex.try_lock()

-- alternative
local mutex = effil.mutex()
local smutex = effil.mutex.spin()
local rmutex = effil.mutex.recursive()
local shmutex = effil.mutex.shared()