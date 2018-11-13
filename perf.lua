
package.path = "./?.lua;../?.lua;../?.so;libs/u-test/?.lua"
package.cpath = "release/?.so"


local effil = require "effil"
local test = require "u-test"

function mesure_time(f, units)
    if not units then
        units = "ms"
    end

    local start = effil.clock.steady[units]()
    f()
    local stop = effil.clock.steady[units]()
    return stop - start
end

test.shared_table.table_insertions = function (insertions, tbl, elem)
    print("Test table for insertion " ..  tostring(insertions) .. " values")

    local lua_time = mesure_time(function()
        for i = 1, insertions do
            tbl[i] = elem
        end
    end, "ns")

    print("total time: " .. tostring(lua_time / 1000) .. " ms")
    print("per insertion: " .. tostring(lua_time / insertions) .. " ns")
end


test.channel.same_value_2threads = function(nelems, elem)
    local channel = effil.channel()

    local runner = effil.thread(function()
        for i = 1, nelems do
            channel:push(elem)
        end
    end)

    local lua_time = mesure_time(function() runner() end, "ms")

    for i = 1, nelems do
        channel:pop()
    end

    print("pass " .. tostring(nelems) .. " objects: " .. tostring(lua_time / 1000) .. " sec")
end


test.thread.spawn_threads = function(nthreads, wait)
    local threads = {}
    local runner = effil.thread(function() return true end)
    for i = 1, nthreads do
        if wait then
            runner():wait()
        else
            threads[i] = runner()
        end
    end


    for k, thread in pairs(threads) do
        thread:wait()
    end
end


local insertions = 100 * 1000 * 1000;
test.shared_table.table_insertions(insertions, {}, 42)
test.shared_table.table_insertions(insertions, effil.table(), 42)

local short_string = "short"
test.shared_table.table_insertions(insertions, {}, short_string)
test.shared_table.table_insertions(insertions, effil.table(), short_string)

local long_string = "I am very very very very long string"
test.shared_table.table_insertions(insertions, {}, long_string)
test.shared_table.table_insertions(insertions, effil.table(), long_string)

test.shared_table.table_insertions(insertions, effil.table(), effil.table())

local nelems = insertions
test.channel.same_value_2threads(nelems, 42)
test.channel.same_value_2threads(nelems, short_string)
test.channel.same_value_2threads(nelems, long_string)
test.channel.same_value_2threads(nelems, effil.table())

