require "bootstrap-tests"

local effil = effil

test.mutex.tear_down = default_tear_down

test.mutex.locking = function()
    local m = effil.mutex()
    local tbl = effil.table { val = 0, start = false}

    local worker = function(inc)
        while not tbl.start do end

        for _ = 1, 1000 do
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

test.mutex.unique_lock = function()
    local m = effil.mutex()

    m:unique_lock(function()
        test.is_false(m:try_unique_lock(function()end))
        test.is_false(m:try_shared_lock(function()end))
    end)
end

test.mutex.try_unique_lock = function()
    local m = effil.mutex()

    m:try_unique_lock(function()
        test.is_false(m:try_unique_lock(function()end))
        test.is_false(m:try_shared_lock(function()end))
    end)
end

test.mutex.shared_lock = function()
    local m = effil.mutex()

    m:shared_lock(function()
        test.is_true(m:try_shared_lock(function()end))
        test.is_false(m:try_unique_lock(function()end))
    end)
end

test.mutex.try_shared_lock = function()
    local m = effil.mutex()

    m:try_shared_lock(function()
        test.is_true(m:try_shared_lock(function()end))
        test.is_false(m:try_unique_lock(function()end))
    end)
end

test.mutex.error_under_lock = function()
    local m = effil.mutex()

    local ret = pcall(m.unique_lock, m, function()
        error('wow')
    end)
    test.is_false(ret)
    test.is_true(m:try_unique_lock(function()end))
end
