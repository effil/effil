require "bootstrap-tests"

test.cache.tear_down = default_tear_down

test.cache.single_thread = function()
    -- Cache is enabled and clear by default
    test.is_true(effil.cache.enabled())
    test.equal(effil.cache.size(), 0)

    -- Check function placed into table goes to cache
    local foo = function() return 33 end
    local t = effil.table({f = foo})
    test.equal(effil.cache.size(), 2)

    -- Check function placed into cache still works and it's the same function
    test.equal(t.f(), 33)
    test.equal(t.f, foo)
    test.equal(effil.cache.size(), 2)

    -- Check cache clearing works
    effil.cache.clear()
    test.equal(effil.cache.size(), 0)

    -- Check function available after cache was cleared
    test.equal(t.f(), 33)
    test.not_equal(t.f, foo)
    test.equal(effil.cache.size(), 2)

    -- Check function available after caching was disabled
    effil.cache.disable()
    test.equal(effil.cache.size(), nil)
    test.equal(t.f(), 33)
    test.equal(effil.cache.size(), nil)

    -- Check cache works if it was enabled back
    effil.cache.enable()
    test.equal(t.f(), 33)
    test.equal(effil.cache.size(), 2)

    -- Check function is cached once
    test.equal(t.f(), 33)
    test.equal(effil.cache.size(), 2)
end

test.cache.simple_thread = function()
    local function foo()
        return require("effil").cache.size()
    end

    local tc_size = effil.thread(foo)():get()
    test.equal(tc_size, 2)
    test.equal(effil.cache.size(), 2)
end

test.cache.speed_comparison = function()
    local function worker(control, storage, disable)
        effil = require "effil"
        if disable then
            effil.cache.disable()
        end
        while not control.start do end-- wait to start

        local iter = 0
        while not control.stop do -- wait to stop
            pcall(storage.func) -- load function
            iter = iter + 1
        end
        return iter
    end

    local loadstring = loadstring or load
    local big_function = loadstring("return function() if false then " .. string.rep("a = {} ", 1000) .. " end end")()
    test.is_true(type(big_function) == "function")

    local control = effil.table()

    local t1 = effil.thread(worker)(control, {func = big_function}, false)
    local t2 = effil.thread(worker)(control, {func = big_function}, true)

    effil.sleep(2) -- give threads time to start
    control.start = true
    effil.sleep(5)
    control.stop = true

    local t1_res = t1:get()
    local t2_res = t2:get()

    print("Thread with cache:    " .. t1_res)
    print("Thread without cache: " .. t2_res)

    test.is_true(t1_res > 0)
    test.is_true(t2_res > 0)
    test.is_true(t1_res > t2_res)

    print("Cache speedup:        " .. ((t1_res / t2_res - 1) * 100).. "%")
end

test.cache.check_capacity = function()
    local loadstring = loadstring or load
    local function check_capacity(capacity)
        test.equal(effil.cache.capacity(), capacity)
        local use_size = capacity > 0 and capacity or 1000

        local t = effil.table()
        for i = 1, use_size do
            local foo = loadstring("return function() end")()
            t[i] = foo
        end
        test.equal(effil.cache.size(), use_size * 2)

        if capacity > 0 then
            for i = 1, 10 do
                local function foo() end
                t[i + capacity] = foo
            end
            test.equal(effil.cache.size(), capacity * 2)
        end
    end

    local default = 1024
    check_capacity(default) -- default

    test.equal(effil.cache.capacity(10), default)
    test.equal(effil.cache.size(), 20)

    effil.cache.clear()
    check_capacity(10)

    test.equal(effil.cache.capacity(200), 10)
    test.equal(effil.cache.size(), 20)

    check_capacity(200)
    effil.cache.clear()

    test.equal(effil.cache.capacity(0), 200)
    check_capacity(0) -- unlimited
end

test.cache.check_upvalue_update = function()
    local upvalue = 2
    local foo = function() return upvalue end
    local tbl = effil.table({foo = foo})
    local thr = effil.thread(function(t) return t.foo() end)

    test.equal(tbl.foo(), 2)
    test.equal(thr(tbl):get(), 2)

    debug.setupvalue(foo, 1, 3)

    test.equal(tbl.foo(), 3)
    test.equal(thr(tbl):get(), 2)

    tbl.foo = foo
    test.equal(thr(tbl):get(), 2)

    effil.cache.remove(foo)
    tbl.foo = foo
    test.equal(thr(tbl):get(), 3)
end
