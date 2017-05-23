local effil = require 'effil'

TestType = { tearDown = tearDown }

function TestType:testType()
    test.assertEquals(effil.type(1), "number")
    test.assertEquals(effil.type("string"), "string")
    test.assertEquals(effil.type(effil.table()), "effil.table")
    test.assertEquals(effil.type(effil.channel()), "effil.channel")
    test.assertEquals(effil.type(effil.thread(function() end)()), "effil.thread")
end
