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

test.gc.store_same_value = function()
    local fill = function (c)
        local a = effil.table {}
        c:push(a)
        c:push(a)
    end

    local c = effil.channel()
    fill(c)

    c:pop()
    collectgarbage()
    effil.gc.collect()
    c:pop()[1] = 0
end
