require "bootstrap-tests"

test.func.tear_down = default_tear_down

test.func.simple = function()
    local t = effil.table()
    t["tonumber"] = tonumber

    test.not_equal(t["tonumber"], tonumber)
    test.equal(t["tonumber"]("123"), tonumber("123"))
    test.equal(effil.thread(function() return t["tonumber"]("123") end)():get(), tonumber("123"))

    local foo = tonumber
    test.equal(foo, tonumber)
    test.equal(foo("123"), tonumber("123"))
    test.equal(effil.thread(function() return foo("123") end)():get(), tonumber("123"))
end