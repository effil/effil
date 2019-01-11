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

test.mutex.error_under_lock = function()
    local m = effil.mutex()

    local ret = pcall(m.unique_lock, m, function()
        error('wow')
    end)
    test.is_false(ret)
    test.is_true(m:try_unique_lock(function()end))
end
