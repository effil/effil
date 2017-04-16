local effil = require "effil"
local gc = effil.gc

TestGC = { tearDown = tearDown }

function TestGC:testCleanup()
    collectgarbage()
    gc.collect()
    test.assertEquals(gc.count(), 1)

    for i = 0, 10000 do
        local tmp = effil.table()
    end

    collectgarbage()
    gc.collect()
    test.assertEquals(gc.count(), 1)
end

function TestGC:testDisableGC()
    local nobjects = 10000

    collectgarbage()
    gc.collect()
    test.assertEquals(gc.count(), 1)

    gc.pause()
    test.assertFalse(gc.enabled())

    for i = 1, nobjects do
        local tmp = effil.table()
    end

    test.assertEquals(gc.count(), nobjects + 1)
    collectgarbage()
    gc.collect()
    test.assertEquals(gc.count(), 1)

    gc.resume()
end