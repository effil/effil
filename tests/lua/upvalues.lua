require "bootstrap-tests"

test.upvalues.tear_down = default_tear_down

test.upvalues.check_single_upvalue_p = function(type_creator, type_checker)
    local obj = type_creator()
    local thread_worker = function(checker) return require("effil").type(obj) .. ": " .. checker(obj) end
    local ret = effil.thread(thread_worker)(type_checker):get()

    print("Returned: " .. ret)
    test.equal(ret, effil.type(obj) .. ": " .. type_checker(obj))
end

local foo = function() return 22 end

test.upvalues.check_single_upvalue_p(function() return 1488 end,
                                   function() return "1488" end)

test.upvalues.check_single_upvalue_p(function() return "awesome" end,
                                   function() return "awesome" end)

test.upvalues.check_single_upvalue_p(function() return true end,
                                   function() return "true" end)

test.upvalues.check_single_upvalue_p(function() return nil end,
                                   function() return "nil" end)

test.upvalues.check_single_upvalue_p(function() return foo end,
                                   function(f) return f() end)

test.upvalues.check_single_upvalue_p(function() return effil.table({key = 44}) end,
                                   function(t) return t.key end)

test.upvalues.check_single_upvalue_p(function() local c = effil.channel() c:push(33) c:push(33) return c end,
                                   function(c) return c:pop() end)

test.upvalues.check_single_upvalue_p(function() return effil.thread(foo)() end,
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

test.upvalues.check_global_env = function()
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

if LUA_VERSION > 51 then

test.upvalues.check_custom_env = function()
    local function create_foo()
        local _ENV = { key = 'value' }
        return function()
            return key
        end
    end

    local foo = create_foo()
    local ret = effil.thread(foo)():get()
    test.equal(ret, 'value')
end

end -- LUA_VERSION > 51
