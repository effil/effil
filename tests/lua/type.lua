local effil = require 'effil'

TestType = { tearDown = tearDown }

function TestType:testType()
    test.assertEquals(effil.type(1), "number")
    test.assertEquals(effil.type("string"), "string")
    test.assertEquals(string.find(effil.type(effil.table()), "effil.table: "), 1)
    test.assertEquals(string.find(effil.type(effil.channel()), "effil.channel: "), 1)
    test.assertEquals(string.find(effil.type(effil.thread(function() end)()), "effil.thread: "), 1)
end
