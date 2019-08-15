require "bootstrap-tests"

test.type.tear_down = default_tear_down

test.type.check_all_types = function()
    test.equal(effil.type(1), "number")
    test.equal(effil.type("string"), "string")
    test.equal(effil.type(true), "boolean")
    test.equal(effil.type(nil), "nil")
    test.equal(effil.type(function()end), "function")
    test.equal(effil.type(tonumber), "function") -- check C function
    test.equal(effil.type(effil.table()), "effil.table")
    test.equal(effil.type(effil.channel()), "effil.channel")

    local thr = effil.thread(function() end)()
    test.equal(effil.type(thr), "effil.thread")
    thr:wait()
end
