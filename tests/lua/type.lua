require "bootstrap-tests"

test.type.tear_down = default_tear_down

test.type = function()
    test.equal(effil.type(1), "number")
    test.equal(effil.type("string"), "string")
    test.equal(effil.type(effil.table()), "effil.table")
    test.equal(effil.type(effil.channel()), "effil.channel")
    local thr = effil.thread(function() end)()
    test.equal(effil.type(thr), "effil.thread")
    thr:wait()
end
