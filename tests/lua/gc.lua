local effil = require "effil"
local gc = effil.gc

TestGC = {tearDown = tearDown }

function TestGC:testCleanup()
    gc.collect()
    test.assertEquals(gc.count(), 0)

    for i = 0, 10000 do
        local tmp = effil.table()
    end

    gc.collect()
    test.assertEquals(gc.count(), 0)
end

function TestGC:testDisableGC()
    local nobjects = 10000
    gc.collect()
    test.assertEquals(gc.count(), 0)

    gc.pause()
    test.assertEquals(gc.status(), "paused")

    for i = 1, nobjects do
        local tmp = effil.table()
    end

    test.assertEquals(gc.count(), nobjects)
    gc.collect()
    test.assertEquals(gc.count(), nobjects)

    gc.resume()
    gc.collect()
    test.assertEquals(gc.count(), 0)
end