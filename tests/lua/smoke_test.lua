TestSmoke = {}

function TestSmoke:tearDown()
    log "TearDown() collect garbage"
    collectgarbage()
end

function TestSmoke:testSharedTableTypes()
    local effil = require('libeffil')
    local share = effil.table()

    share["number"] = 100500
    share["string"] = "string value"
    share["bool"] = true
    share["function"] = function(left, right) return left + right end

    local thread_factory = effil.thread(
        function(share)
            share["child.number"]   = share["number"]
            share["child.string"]   = share["string"]
            share["child.bool"]     = share["bool"]
            share["child.function"] = share["function"](11,45)
        end
    )
    local thread = thread_factory(share)
    thread:wait()

    log "Check values"
    test.assertEquals(share["child.number"], share["number"])
    test.assertEquals(share["child.string"], share["string"])
    test.assertEquals(share["child.bool"], share["bool"])
    test.assertEquals(share["child.function"], share["function"](11,45))
end

function TestSmoke:testThreadCancel()
    local effil = require('libeffil')
    local thread_runner = effil.thread(
        function()
            local startTime = os.time()
            while ( (os.time() - startTime) <= 10) do --[[ Just sleep ]] end
        end
    )
    test.assertFalse(thread_runner:stepwise(true))
    local thread = thread_runner()
    sleep(2) -- let thread starts working
    thread:cancel()

    test.assertTrue(wait(2, function() return thread:status() ~= 'running' end))
    test.assertEquals(thread:status(), 'canceled')
end

function TestSmoke:testThreadPauseAndResume()
    local effil = require('libeffil')
    local data = effil.table()
    data.value = 0
    local thread_runner = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )
    test.assertFalse(thread_runner:stepwise(true))

    local thread = thread_runner(data)
    test.assertTrue(wait(2, function() return data.value > 100 end))
    thread:pause()
    test.assertTrue(wait(2, function() return thread:status() == "paused" end))
    local savedValue = data.value
    sleep(3)
    test.assertEquals(data.value, savedValue)

    thread:resume()
    test.assertTrue(wait(5, function() return (data.value - savedValue) > 100 end))
    thread:cancel()
    thread:wait()
end

function TestSmoke:testThreadPauseAndStop()
    local effil = require('libeffil')
    log "Create thread"
    local data = effil.table()
    data.value = 0
    local thread_runner = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )
    test.assertFalse(thread_runner:stepwise(true))

    local thread = thread_runner(data)
    test.assertTrue(wait(2, function() return data.value > 100 end))
    thread:pause()
    test.assertTrue(wait(2, function() return thread:status() == "paused" end))
    local savedValue = data.value
    sleep(3)
    test.assertEquals(data.value, savedValue)

    thread:cancel()
    test.assertTrue(wait(2, function() return thread:status() == "canceled" end))
    thread:wait()
end

function TestSmoke:testThreadPauseAndStop()
    local effil = require('libeffil')
    log "Create thread"
    local data = effil.table()
    data.value = 0
    local thread_runner = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )
    test.assertFalse(thread_runner:stepwise(true))

    local thread = thread_runner(data)
    test.assertTrue(wait(2, function() return data.value > 100 end))
    thread:pause()
    test.assertTrue(wait(2, function() return thread:status() == "paused" end))
    local savedValue = data.value
    sleep(3)
    test.assertEquals(data.value, savedValue)

    thread:cancel()
    test.assertTrue(wait(2, function() return thread:status() == "canceled" end))
    thread:wait()
end

function TestSmoke:testRecursiveTables()
    local effil = require('libeffil')
    local share = effil.table()

    local magic_number = 42
    share["subtable1"] = effil.table()
    share["subtable1"]["subtable1"] = effil.table()
    share["subtable1"]["subtable2"] = share["subtable1"]["subtable1"]
    share["subtable2"] = share["subtable1"]["subtable1"]
    share["magic_number"] = magic_number

    local thread_factory = effil.thread(
        function(share)
            share["subtable1"]["subtable1"]["magic_number"] = share["magic_number"]
            share["magic_number"] = nil
        end
    )
    local thread = thread_factory(share)
    thread:wait()

    log "Check values"
    test.assertEquals(share["subtable1"]["subtable1"]["magic_number"], magic_number)
    test.assertEquals(share["subtable1"]["subtable2"]["magic_number"], magic_number)
    test.assertEquals(share["subtable2"]["magic_number"], magic_number)
    test.assertEquals(share["magic_number"], nil)
end

function TestSmoke:testThisThreadFunctions()
    local effil = require('libeffil')
    local share = effil.table()

    local thread_factory = effil.thread(
        function(share)
            share["child.id"] = require('libeffil').thread_id()
        end
    )
    local thread = thread_factory(share)
    thread:wait()

    log "Check values"
    test.assertString(share["child.id"])
    test.assertNumber(tonumber(share["child.id"]))
    test.assertNotEquals(share["child.id"], effil.thread_id())
    effil.yield() -- just call it

    local function check_time(real_time, use_time, metric)
        local start_time = os.time()
        effil.sleep(use_time, metric)
        test.assertAlmostEquals(os.time(), start_time + real_time, 1)
    end
    check_time(4, 4, nil) -- seconds by default
    check_time(4, 4, 's')
    check_time(4, 4000, 'ms')
    check_time(60, 1, 'm')
end

function TestSmoke:testCheckThreadReturns()
    local effil = require('libeffil')
    local share = effil.table()
    share.value = "some value"

    local thread_factory = effil.thread(
        function(share)
            return 100500, "string value", true, share, function(a,b) return a + b end
        end
    )
    local thread = thread_factory(share)
    local status, returns = thread:wait()

    log "Check values"
    test.assertEquals(status, "completed")

    test.assertNumber(returns[1])
    test.assertEquals(returns[1], 100500)

    test.assertString(returns[2])
    test.assertEquals(returns[2], "string value")

    test.assertBoolean(returns[3])
    test.assertTrue(returns[3])

    test.assertUserdata(returns[4])
    test.assertEquals(returns[4].value, share.value)

    test.assertFunction(returns[5])
    test.assertEquals(returns[5](11, 89), 100)
end

function TestSmoke:testCheckPairsInterating()
    local effil = require('libeffil')
    local share = effil.table()
    local data = { 0, 0, 0, ["key1"] = 0, ["key2"] = 0, ["key3"] = 0 }

    for k, _ in pairs(data) do
        share[k] = k .. "-value"
    end

    for k,v in pairs(share) do
        test.assertEquals(data[k], 0)
        data[k] = 1
        test.assertEquals(v, k .. "-value")
    end

    for k,v in pairs(data) do
        log("Check: " .. k)
        test.assertEquals(v, 1)
    end

    for k,v in ipairs(share) do
        test.assertEquals(data[k], 1)
        data[k] = 2
        test.assertEquals(v, k .. "-value")
    end

    for k,v in ipairs(data) do
        log("Check: " .. k)
        test.assertEquals(v, 2)
    end
end

function TestSmoke:testCheckLengthOperator()
    local effil = require('libeffil')
    local share = effil.table()
    share[1] = 10
    share[2] = 20
    share[3] = 30
    share[4] = 40
    share["str"] = 50
    log "Check values"
    test.assertEquals(#share, 4)
    share[3] = nil
    test.assertEquals(#share, 2)
    share[1] = nil
    test.assertEquals(#share, 0)
end
