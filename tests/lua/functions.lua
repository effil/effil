require "bootstrap-tests"

test.type.tear_down = default_tear_down

test.functions.check_C_function = function()
    local t = effil.table()
    t[1] = tonumber -- C function
    test.equal(t[1]("33"), 33)
end
