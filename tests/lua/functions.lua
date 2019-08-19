require "bootstrap-tests"

test.type.tear_down = default_tear_down

if jit then
    test.functions.check_C_function = function()
        local t = effil.table()

        -- 'tonumber' is not real function in luajit
        local ret = pcall(function() t[1] = tonumber end)
        test.equal(ret, false)

        -- valid C function even in luajit
        t[1] = package.loaders[3]
        ret = t[1]('effil') -- should get loader function
        test.equal(type(ret), 'function')
    end
else
    test.functions.check_C_function = function()
        local t = effil.table()
        t[1] = tonumber -- C function
        test.equal(t[1]("33"), 33)
    end
end
