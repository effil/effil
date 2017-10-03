require "bootstrap-tests"

test.upvalues.tear_down = default_tear_down

test.upvalues.check_single_upvalue = function(type_creator, type_checker)
    local obj = type_creator()
    local thread_worker = function(checker) return require("effil").type(obj) .. ": " .. checker(obj) end
    local ret = effil.thread(thread_worker)(type_checker):get()

    print("Returned: " .. ret)
    test.equal(ret, effil.type(obj) .. ": " .. type_checker(obj))
end

local foo = function() return 22 end

test.upvalues.check_single_upvalue(function() return 1488 end,
                                   function() return "1488" end)

test.upvalues.check_single_upvalue(function() return "awesome" end,
                                   function() return "awesome" end)

test.upvalues.check_single_upvalue(function() return true end,
                                   function() return "true" end)

test.upvalues.check_single_upvalue(function() return nil end,
                                   function() return "nil" end)

test.upvalues.check_single_upvalue(function() return foo end,
                                   function(f) return f() end)

test.upvalues.check_single_upvalue(function() return effil.table({key = 44}) end,
                                   function(t) return t.key end)

test.upvalues.check_single_upvalue(function() local c = effil.channel() c:push(33) c:push(33) return c end,
                                   function(c) return c:pop() end)

test.upvalues.check_single_upvalue(function() return effil.thread(foo)() end,
                                   function(t) return t:get() end)

test.upvalues.check_invalid_coroutine = function()
    local obj = coroutine.create(foo)
    local thread_worker = function() return tostring(obj) end
    local ret, err = pcall(effil.thread(thread_worker))
    if ret then
        ret:wait()
    end
    test.is_false(ret)
    print("Returned: " .. err)
    upvalue_num = LUA_VERSION > 51 and 2 or 1
    test.equal(err, "effil.thread: bad function upvalue #" .. upvalue_num ..
          " (unable to store object of thread type)")
end

test.upvalues.check_table = function()
    local obj = { key = "value" }
    local thread_worker = function() return require("effil").type(obj) .. ": " .. obj.key end
    local ret = effil.thread(thread_worker)():get()

    print("Returned: " .. ret)
    test.equal(ret, "effil.table: value")
end

test.upvalues.check_env = function()
    local obj1 = 13 -- local
    obj2 = { key = "origin" } -- global
    local obj3 = 79 -- local

    local function foo() -- _ENV is 2nd upvalue
        return obj1, obj2.key, obj3
    end

    local function thread_worker(func)
        obj1 = 31 -- global
        obj2 = { key = "local" } -- global
        obj3 = 97 -- global
        return table.concat({func()}, ", ")
    end

    local ret = effil.thread(thread_worker)(foo):get()
    print("Returned: " .. ret)
    test.equal(ret, "13, local, 79")
end

local function check_works(should_work)
    local obj = { key = "value"}
    local function worker()
        return obj.key
    end

    local ret, err = pcall(effil.thread(worker))
    if ret then
        err:wait()
    end
    test.equal(ret, should_work)
    if not should_work then
        test.equal(err, "effil.thread: bad function upvalue #1 (table is disabled by effil.allow_table_upvalue)")
    end
end

test.upvalues_table.tear_down = function()
    effil.allow_table_upvalue(true)
    default_tear_down()
end

test.upvalues_table.disabling_table_upvalues = function()
    test.equal(effil.allow_table_upvalue(), true)
    -- works by default
    check_works(true)

    -- disable
    test.equal(effil.allow_table_upvalue(false), true)
    check_works(false)
    test.equal(effil.allow_table_upvalue(), false)

    -- enable back
    test.equal(effil.allow_table_upvalue(true), false)
    check_works(true)
    test.equal(effil.allow_table_upvalue(), true)
end
