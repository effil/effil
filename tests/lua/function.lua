require "bootstrap-tests"

test.func.tear_down = default_tear_down

test.func.check_truly_c_functions_p = function(func)
    test.equal(type(func), "function")

    local t = effil.table()
    local ret, _ = pcall(function() t["func"] = func end)
    test.equal(ret, true)
end

test.func.check_truly_c_functions_p(coroutine.create)
test.func.check_truly_c_functions_p(effil.size)

test.func.check_tosting_c_functions = function()
    local t = effil.table()

    if jit then
        -- in LuaJIT it's not real C function
        local ret, msg = pcall(function() t["tostring"] = tostring end)
        test.equal(ret, false)
        test.equal(msg, "effil.table: can't get C function pointer")
    else
        t["tostring"] = tostring

        if LUA_VERSION > 51 then
            test.equal(t["tostring"], tostring)
        else
            test.not_equal(t["tostring"], tostring)
        end -- LUA_VERSION > 51

        test.equal(t["tostring"](123), tostring(123))
        test.equal(effil.thread(function() return t["tostring"](123) end)():get(), tostring(123))

        local foo = tostring
        test.equal(foo, tostring)
        test.equal(foo(123), tostring(123))
        test.equal(effil.thread(function() return foo(123) end)():get(), tostring(123))
    end -- jit
end


