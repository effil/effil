require "bootstrap-tests"

local gc = effil.gc

test.gc.tear_down = default_tear_down

test.gc.cleanup = function ()
    collectgarbage()
    gc.collect()
    test.equal(gc.count(), 1)

    for i = 0, 10000 do
        local tmp = effil.table()
    end

    collectgarbage()
    gc.collect()
    test.equal(gc.count(), 1)
end

test.gc.disable = function ()
    local nobjects = 10000

    collectgarbage()
    gc.collect()
    test.equal(gc.count(), 1)

    gc.pause()
    test.is_false(gc.enabled())

    for i = 1, nobjects do
        local tmp = effil.table()
    end

    test.equal(gc.count(), nobjects + 1)
    collectgarbage()
    gc.collect()
    test.equal(gc.count(), 1)

    gc.resume()
end