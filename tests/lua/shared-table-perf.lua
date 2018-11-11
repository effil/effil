require "bootstrap-tests"

local function mesure_time(f, units)
    if not units then
        units = "ms"
    end

    local start = effil.clock.steady[units]()
    f()
    local stop = effil.clock.steady[units]()
    return stop - start
end

test.perf.table_insertions = function (insertions, tbl, elem)
    print("Test table for insertion " ..  tostring(insertions) .. " values")

    local lua_time = mesure_time(function()
        for i = 1, insertions do
            tbl[i] = elem
        end
    end, "ns")

    print("total time: " .. tostring(lua_time / 1000) .. " ms")
    print("per insertion: " .. tostring(lua_time / insertions) .. " ns")
end

local insertions = 100 * 1000 * 1000;
test.perf.table_insertions(insertions, {}, 42)
test.perf.table_insertions(insertions, effil.table(), 42)
test.perf.table_insertions(insertions, {}, "effil")
test.perf.table_insertions(insertions, effil.table(), "effil")