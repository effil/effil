require "bootstrap-tests"

test.type = function()
    test.equal(effil.type(1), "number")
    test.equal(effil.type("string"), "string")
    test.equal(effil.type(effil.table()), "effil.table")
    test.equal(effil.type(effil.channel()), "effil.channel")
    test.equal(effil.type(effil.thread(function() end)()), "effil.thread")
end
