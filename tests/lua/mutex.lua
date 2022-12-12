require "bootstrap-tests"

local effil = effil

test.mutex.tear_down = default_tear_down

test.mutex.locking = function()
    local m = effil.mutex()
    local tbl = effil.table { val = 0, start = false}

    local worker = function(inc)
        while not tbl.start do end

        for i = 1, 1000 do
            m:unique_lock(function()
                if inc then
                    tbl.val = tbl.val + 1
                else
                    tbl.val = tbl.val - 1
                end
            end)
        end
    end

    local threadInc = effil.thread(worker)(true)
    local threadDec = effil.thread(worker)(false)
    tbl.start = true
    test.equal(threadInc:wait(), "completed")
    test.equal(threadDec:wait(), "completed")

    test.equal(tbl.val, 0)
end

local function try_lock(m, lock_type)
    return m['try_' .. lock_type .. '_lock'](m, function() end)
end

local function lock(m, lock_type)
    local ctl = {}
    local state = effil.table { do_lock = true, on_lock = false }

    function ctl.unlock()
        state.do_lock = false
        test.equal(ctl.thread:wait(), "completed")
    end

    ctl.thread = effil.thread(function()
        m[lock_type .. '_lock'](m, function()
            state.on_lock = true
            while state.do_lock do end
        end)
        state.on_lock = false
    end)()

    test.is_true(wait(2, function() return state.on_lock end))
    return ctl
end

test.mutex.unique_lock = function()
    local m = effil.mutex()

    local l = lock(m, 'unique')
    test.is_false(try_lock(m, 'unique'))
    test.is_false(try_lock(m, 'shared'))
    l:unlock()
end

test.mutex.try_unique_lock = function()
    local m = effil.mutex()

    local l = lock(m, 'try_unique')
    test.is_false(try_lock(m, 'unique'))
    test.is_false(try_lock(m, 'shared'))
    l:unlock()
end

test.mutex.shared_lock = function()
    local m = effil.mutex()

    local l = lock(m, 'shared')
    test.is_true(try_lock(m,  'shared'))
    test.is_false(try_lock(m, 'unique'))
    l:unlock()
end

test.mutex.try_shared_lock = function()
    local m = effil.mutex()

    local l = lock(m, 'try_shared')
    test.is_true(try_lock(m,  'shared'))
    test.is_false(try_lock(m, 'unique'))
    l:unlock()
end

test.mutex.error_under_lock = function(lock_type)
    local m = effil.mutex()
    local err_msg = "wow"

    local ret, msg = pcall(m.unique_lock, m, function()
        error(err_msg)
    end)
    test.is_false(ret)
    test.is_not_nil(string.find(msg, ".+: " .. err_msg))
    test.is_true(m:try_unique_lock(function()end))
end

test.mutex.cancel_under_lock_p = function(lock_type)
    local chan = effil.channel()
    local m = effil.mutex()

    local thr = effil.thread(function()
        m[lock_type](m, function()
            while true do
                effil.yield()
                chan:push(1)
            end
        end)
    end)()

    test.equal(chan:pop(1), 1) -- wait until thread is runned
    test.is_true(thr:cancel(1))
    test.equal(thr:status(), "cancelled")
    test.is_true(m:try_unique_lock(function()end))
end

test.mutex.cancel_while_lock_p = function(lock_type)
    local tbl = effil.table()
    local m = effil.mutex()

    local lock_thr = effil.thread(function() -- Lock mutex
        m:unique_lock(function()
            while not tbl.stop do
            end
        end)
    end)()

    local hang_thr = effil.thread(function() -- hanged thread
        m[lock_type](m, function() end)
    end)()

    test.is_true(hang_thr:cancel(2))
    test.equal(hang_thr:status(), "cancelled")
    tbl.stop = true
    test.is_true(lock_thr:wait() == "completed")
end

for _, lock_type in ipairs {"unique_lock", "shared_lock"} do
    test.mutex.cancel_under_lock_p(lock_type)
    test.mutex.cancel_while_lock_p(lock_type)
end