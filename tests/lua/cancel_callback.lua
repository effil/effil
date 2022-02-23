require "bootstrap-tests"

local effil = effil

test.thread_cancel_callback.tear_down = default_tear_down

test.thread_cancel_callback.on_main_thread = function ()
    test.equal(effil.cancel_callback, nil)

    local foo = function() end
    effil.cancel_callback = foo
    test.equal(effil.cancel_callback, nil)
end

test.thread_cancel_callback.called = function ()
    local ctx = effil.table()
    local thread = effil.thread(function()
        effil.cancel_callback = function()
            ctx.called = true
        end
        while true do
            effil.yield()
        end
    end)()

    test.is_true(thread:cancel())
    test.equal(thread:status(), "canceled")
    test.is_true(ctx.called)
end

test.thread_cancel_callback.failed = function ()
    local curr_file = debug.getinfo(1,'S').short_src
    local thread = effil.thread(function()
        effil.cancel_callback = function()
            error("something happend")
        end
        while true do
            effil.yield()
        end
    end)()

    test.is_true(thread:cancel())

    local status, msg, trace = thread:status()
    test.equal(status, "failed")
    test.is_not_nil(string.find(msg, curr_file .. ":%d+: something happend"))

    if LUA_VERSION > 51 then
        print(trace)
        local regex = (
[[stack traceback:
%%s%%[C%%]: in function 'error'
%%s%s:%%d+: in function <%s:%%d+>
%%s%%[C%%]: in %%S+ 'yield'
%%s%s:%%d+: in function <%s:%%d+>]]
        ):format(curr_file, curr_file, curr_file, curr_file)
        test.is_not_nil(string.find(trace, regex))
    end
end
