require "bootstrap-tests"

local effil = effil

test.thread.tear_down = default_tear_down

test.thread.hardware_threads = function()
    test.is_true(effil.hardware_threads() >= 0)
end

test.thread.runner_is_serializible = function ()
    local table = effil.table()
    local runner = effil.thread(function(n) return n * 2 end)

    table["runner"] = runner
    test.equal(table["runner"](123):get(), 246)
end

test.thread.runner_path_check_p = function (config_key, pkg)
    local table = effil.table()
    local runner = effil.thread(function()
        require(pkg)
    end)
    test.equal(runner():wait(), "completed")

    runner[config_key] = ""
    test.equal(runner():wait(), "failed")
end

test.thread.runner_path_check_p("path", "size") -- some testing Lua file to import
test.thread.runner_path_check_p("cpath", "effil")

test.thread.wait = function ()
    local thread = effil.thread(function()
        print 'Effil is not that tower'
        return nil end)()

    local status = thread:wait()
    test.is_nil(thread:get())
    test.equal(status, "completed")
    test.equal(thread:status(), "completed")
end

test.thread.multiple_wait_get = function ()
    local thread = effil.thread(function() return "test value" end)()
    local status1 = thread:wait()
    local status2 = thread:wait()
    test.equal(status1, "completed")
    test.equal(status2, status1)

    local value = thread:get()
    test.equal(value, "test value")
end

test.thread.timed_get = function ()
    local thread = effil.thread(function()
        effil.sleep(2)
        return "-_-"
    end)()
    test.is_nil(thread:get(1))
    test.equal(thread:get(2), "-_-")
end

test.thread.timed_get = function ()
    local thread = effil.thread(function()
        effil.sleep(2)
        return 8
    end)()

    local status = thread:wait(1)
    test.equal(status, "running")

    local value = thread:get(2, "s")
    test.equal(value, 8);

    test.equal(thread:status(), "completed")
end

test.thread.async_wait = function()
    local thread = effil.thread( function()
        effil.sleep(1)
    end)()

    local iter = 0
    while thread:wait(0) == "running" do
        iter = iter + 1
    end

    test.is_true(iter > 10)
    test.equal(thread:status(), "completed")
end

test.thread.detached = function ()
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
        test.equal(st[i], i)
    end
end

-- FIXME: what is it for?
test.thread.cancel = function ()
    local thread = effil.thread(
        jit ~= nil and
            function()
                while true do
                    effil.yield()
                end
            end
        or
            function()
                while true do end
            end
    )()

    test.is_true(thread:cancel())
    test.equal(thread:status(), "cancelled")
end

test.thread.async_cancel = function ()
    local thread_runner = effil.thread(
        function()
            local startTime = os.time()
            while ( (os.time() - startTime) <= 10) do --[[ Just sleep ]] end
        end
    )

    local thread = thread_runner()
    sleep(2) -- let thread starts working
    thread:cancel(0)

    test.is_true(wait(2, function() return thread:status() ~= 'running' end))
    test.equal(thread:status(), 'cancelled')
end

test.thread.pause_resume_cancel = function ()
    local data = effil.table()
    data.value = 0
    local thread = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )(data)
    test.is_true(wait(2, function() return data.value > 100 end))
    test.is_true(thread:pause())
    test.equal(thread:status(), "paused")
    
    local savedValue = data.value
    sleep(1)
    test.equal(data.value, savedValue)

    thread:resume()
    test.is_true(wait(5, function() return (data.value - savedValue) > 100 end))
    test.is_true(thread:cancel())
end

test.thread.pause_cancel = function ()
    local data = effil.table()
    data.value = 0
    local thread = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )(data)

    test.is_true(wait(2, function() return data.value > 100 end))
    thread:pause(0)
    test.is_true(wait(2, function() return thread:status() == "paused" end))
    local savedValue = data.value
    sleep(1)
    test.equal(data.value, savedValue)

    test.is_true(thread:cancel(1))
end

test.thread.async_pause_resume_cancel = function ()
    local data = effil.table()
    data.value = 0
    local thread = effil.thread(
        function(data)
            while true do
                data.value = data.value + 1
            end
        end
    )(data)

    test.is_true(wait(2, function() return data.value > 100 end))
    thread:pause()

    local savedValue = data.value
    sleep(1)
    test.equal(data.value, savedValue)

    thread:resume()
    test.is_true(wait(5, function() return (data.value - savedValue) > 100 end))

    thread:cancel(0)
    test.is_true(wait(5, function() return thread:status() == "cancelled" end))
    thread:wait()
end

test.thread.returns = function ()
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

    test.equal(status, "completed")

    test.is_number(returns[1])
    test.equal(returns[1], 100500)

    test.is_string(returns[2])
    test.equal(returns[2], "string value")

    test.is_boolean(returns[3])
    test.is_true(returns[3])

    test.is_userdata(returns[4])
    test.equal(returns[4].value, share.value)

    test.is_function(returns[5])
    test.equal(returns[5](11, 89), 100)
end

test.thread_with_table.tear_down = default_tear_down

test.thread_with_table.types = function ()
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

    test.equal(share["child.number"], share["number"])
    test.equal(share["child.string"], share["string"])
    test.equal(share["child.bool"], share["bool"])
    test.equal(share["child.function"], share["function"](11,45))
end

test.thread_with_table.recursive = function ()
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

    test.equal(share["subtable1"]["subtable1"]["magic_number"], magic_number)
    test.equal(share["subtable1"]["subtable2"]["magic_number"], magic_number)
    test.equal(share["subtable2"]["magic_number"], magic_number)
    test.equal(share["magic_number"], nil)
end

test.this_thread.tear_down = default_tear_down

test.this_thread.functions = function ()
    local share = effil.table()

    local thread_factory = effil.thread(
        function(share)
            share["child.id"] = effil.thread_id()
        end
    )
    local thread = thread_factory(share)
    thread:get()

    test.is_string(share["child.id"])
    test.is_number(tonumber(share["child.id"]))
    test.not_equal(share["child.id"], effil.thread_id())
end

test.this_thread.cancel_with_yield = function ()
    local ctx = effil.table()
    local spec = effil.thread(function()
        while not ctx.stop do
           -- Just waiting
        end
        ctx.done = true
        while true do
            effil.yield()
        end
        ctx.after_yield = true
    end)
    spec.step = 0
    local thr = spec()

    test.is_false(thr:cancel(1))
    ctx.stop = true

    test.is_true(thr:cancel())
    test.equal(thr:status(), "cancelled")
    test.is_true(ctx.done)
    test.is_nil(ctx.after_yield)
end

test.this_thread.pause_with_yield = function ()
    local share = effil.table({stop = false})
    local spec = effil.thread(function (share)
        while not share.stop do
            effil.yield()
        end
        share.done = true
        return true
    end)
    spec.step = 0
    local thr = spec(share)

    thr:pause()
    test.is_nil(share.done)
    test.equal(thr:status(), "paused")
    share.stop = true
    effil.sleep(100, "ms")
    test.is_nil(share.done)
    thr:resume()

    test.is_true(thr:get())
    test.is_true(share.done)
end

local function worker(cmd)
    eff = effil
    while not cmd.need_to_stop do
        eff.yield()
    end
    return true
end

local function call_pause(thr)
    -- 'pause()' may hang infinitelly, so lets to run it in separate thread
    thr:pause()
    return true
end

-- Regress test to check hanging when invoke pause on cancelled thread
test.this_thread.pause_on_cancelled_thread = function ()
    local worker_thread = effil.thread(worker)({ need_to_stop = false})
    effil.sleep(1, 's')
    worker_thread:cancel()
    test.equal(worker_thread:wait(2, "s"), "cancelled")
    test.is_true(effil.thread(call_pause)(worker_thread):get(5, "s"))
end

-- Regress test to check hanging when invoke pause on finished thread
test.this_thread.pause_on_finished_thread = function ()
    local cmd = effil.table({ need_to_stop = false})
    local worker_thread = effil.thread(worker)(cmd)
    effil.sleep(1, 's')
    cmd.need_to_stop = true
    test.is_true(worker_thread:get(2, "s"))
    test.is_true(effil.thread(call_pause)(worker_thread):get(5, "s"))
end

if LUA_VERSION > 51 then

test.thread.traceback = function()
    local curr_file = debug.getinfo(1,'S').short_src

    local function foo()
        function boom()
            error("err msg")
        end
        function bar()
            boom()
        end
        bar()
    end

    local status, err, trace = effil.thread(foo)():wait()
    print("status: ", status)
    print("error: ", err)
    print("stacktrace:")
    print(trace)

    test.equal(status, "failed")
    -- <souce file>.lua:<string number>: <error message>
    test.is_not_nil(string.find(err, curr_file .. ":%d+: err msg"))
    test.is_not_nil(string.find(trace, (
[[stack traceback:
%%s%%[C%%]: in function 'error'
%%s%s:%%d+: in function 'boom'
%%s%s:%%d+: in function 'bar'
%%s%s:%%d+: in function <%s:%%d+>]]
        ):format(curr_file, curr_file, curr_file, curr_file)
    ))
end

end -- LUA_VERSION > 51

test.thread.cancel_thread_with_pcall = function()
    local steps = effil.table{step1 = false, step2 = false}
    local pcall_results = effil.table{}

    local thr = effil.thread(
        function()
            pcall_results.ret, pcall_results.msg = pcall(function()
                while true do
                    effil.yield()
                end
            end)

            steps.step1 = true
            effil.yield()
            steps.step2 = true -- should never reach
        end
    )()

    test.is_true(thr:cancel())
    test.equal(thr:wait(), "cancelled")
    test.is_true(steps.step1)
    test.is_false(steps.step2)
    test.is_false(pcall_results.ret)
    test.equal(pcall_results.msg, "Effil: thread is cancelled")
end

test.thread.cancel_thread_with_pcall_not_cancelled = function()
    local thr = effil.thread(
        function()
            pcall(function()
                while true do
                    effil.yield()
                end
            end)
        end
    )()
    test.is_true(thr:cancel())
    test.equal(thr:wait(), "completed")
end

test.thread.cancel_thread_with_pcall_and_another_error = function()
    local msg = 'some text'
    local thr = effil.thread(
        function()
            pcall(function()
                while true do
                    effil.yield()
                end
            end)
            error(msg)
        end
    )()
    test.is_true(thr:cancel())
    local status, message = thr:wait()
    test.equal(status, "failed")
    test.is_not_nil(string.find(message, ".+: " .. msg))
end

if not jit then

test.thread.cancel_thread_with_pcall_without_yield = function()
    local thr = effil.thread(
        function()
            while true do
                -- pass
            end
        end
    )
    thr = thr()
    test.is_true(thr:cancel())
    test.equal(thr:wait(), "cancelled")
end

end

test.thread.check_effil_pcall_success = function()
    local inp1, inp2, inp3 = 1, "str", {}
    local res, ret1, ret2, ret3 = effil.pcall(function(...) return ... end, inp1, inp2, inp3)
    test.is_true(res)
    test.equal(ret1, inp1)
    test.equal(ret2, inp2)
    test.equal(ret3, inp3)
end

test.thread.check_effil_pcall_fail = function()
    local err = "some text"
    local res, msg = effil.pcall(function(err) error(err) end, err)
    test.is_false(res)
    test.is_not_nil(string.find(msg, ".+: " .. err))
end

test.thread.check_effil_pcall_with_cancel_thread = function()
    local thr = effil.thread(
        function()
            effil.pcall(function()
                while true do
                    effil.yield()
                end
            end)
        end
    )()
    test.is_true(thr:cancel())
    test.equal(thr:wait(), "cancelled")
end
