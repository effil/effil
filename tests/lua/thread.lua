local effil = require 'effil'

TestThread = {tearDown = tearDown }

function TestThread:testWait()
    local thread = effil.thread(function() print 'Effil is not that tower' end)()
    local status = thread:wait()
    test.assertNil(thread:get())
    test.assertEquals(status, "completed")
    test.assertEquals(thread:status(), "completed")
end

function TestThread:testMultipleWaitGet()
    local thread = effil.thread(function() return "test value" end)()
    local status1 = thread:wait()
    local status2 = thread:wait()
    test.assertEquals(status1, "completed")
    test.assertEquals(status2, status1)

    local value = thread:get()
    test.assertEquals(value, "test value")
end

function TestThread:testTimedGet()
    local thread = effil.thread(function()
        require('effil').sleep(2)
        return "-_-"
    end)()
    test.assertNil(thread:get(1))
    test.assertEquals(thread:get(2), "-_-")
end

function TestThread:testTimedWait()
    local thread = effil.thread(function()
        require('effil').sleep(2)
        return 8
    end)()

    local status = thread:wait(1)
    test.assertEquals(status, "running")

    local value = thread:get(2, "s")
    test.assertEquals(value, 8);

    test.assertEquals(thread:status(), "completed")
end

function TestThread:testAsyncWait()
    local thread = effil.thread( function()
        require('effil').sleep(1)
    end)()

    local iter = 0
    while thread:wait(0) == "running" do
        iter = iter + 1
    end

    test.assertTrue(iter > 10)
    test.assertEquals(thread:status(), "completed")
end

function TestThread:testDetached()
    local st = effil.table()

    for i = 1, 32 do
        effil.thread(function(st, index)
            st[index] = index
        end)(st, i)
    end

    -- here all thead temporary objects have to be destroyed
    collectgarbage()
    effil.sleep(1)

    for i = 1, 32 do
        test.assertEquals(st[i], i)
    end
end

function TestThread:testCancel()
    local thread = effil.thread(function()
        while true do end
    end)()

    test.assertTrue(thread:cancel())
    test.assertEquals(thread:status(), "canceled")
end

function TestThread:testAsyncCancel()
    local thread_runner = effil.thread(
        function()
            local startTime = os.time()
            while ( (os.time() - startTime) <= 10) do --[[ Just sleep ]] end
        end
    )

    local thread = thread_runner()
    sleep(2) -- let thread starts working
    thread:cancel(0)

    test.assertTrue(wait(2, function() return thread:status() ~= 'running' end))
    test.assertEquals(thread:status(), 'canceled')
end

function TestThread:testPauseResumeCancel()
    local data = effil.table()
    data.value = 0
    local thread = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )(data)
    test.assertTrue(wait(2, function() return data.value > 100 end))
    test.assertTrue(thread:pause())
    test.assertEquals(thread:status(), "paused")

    local savedValue = data.value
    sleep(1)
    test.assertEquals(data.value, savedValue)

    thread:resume()
    test.assertTrue(wait(5, function() return (data.value - savedValue) > 100 end))
    test.assertTrue(thread:cancel())
end

function TestThread:testPauseCancel()
    local data = effil.table()
    data.value = 0
    local thread = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )(data)

    test.assertTrue(wait(2, function() return data.value > 100 end))
    thread:pause(0)
    test.assertTrue(wait(2, function() return thread:status() == "paused" end))
    local savedValue = data.value
    sleep(1)
    test.assertEquals(data.value, savedValue)

    test.assertTrue(thread:cancel(0))
end

function TestThread:testAsyncPauseResumeCancel()
    local data = effil.table()
    data.value = 0
    local thread = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )(data)

    test.assertTrue(wait(2, function() return data.value > 100 end))
    thread:pause()

    local savedValue = data.value
    sleep(1)
    test.assertEquals(data.value, savedValue)

    thread:resume()
    test.assertTrue(wait(5, function() return (data.value - savedValue) > 100 end))

    thread:cancel(0)
    test.assertTrue(wait(5, function() return thread:status() == "canceled" end))
end

function TestThread:testCheckThreadReturns()
    local share = effil.table()
    share.value = "some value"

    local thread_factory = effil.thread(
        function(share)
            return 100500, "string value", true, share, function(a,b) return a + b end
        end
    )
    local thread = thread_factory(share)
    local status = thread:wait()
    local returns = { thread:get() }

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

function TestThread:testTimedCancel()
    local thread = effil.thread(function()
        require("effil").sleep(2)
    end)()
    test.assertFalse(thread:cancel(1))
    thread:wait()
end

TestThreadWithTable  = {tearDown = tearDown }

function TestThreadWithTable:testSharedTableTypes()
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

function TestThreadWithTable:testRecursiveTables()
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

TestThisThread = {tearDown = tearDown }

function TestThisThread:testThisThreadFunctions()
    local share = effil.table()

    local thread_factory = effil.thread(
        function(share)
            share["child.id"] = require('effil').thread_id()
        end
    )
    local thread = thread_factory(share)
    thread:get()

    test.assertString(share["child.id"])
    test.assertNumber(tonumber(share["child.id"]))
    test.assertNotEquals(share["child.id"], effil.thread_id())
    effil.yield() -- just call it
end

function TestThisThread:testTime()
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
