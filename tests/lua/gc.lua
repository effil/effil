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

local function create_fabric()
    local f = { data = {} }

    function f:create(num)
        for i = 1, num do
            table.insert(self.data, effil.table())
        end
    end

    function f:remove(num)
        for i = 1, num do
            table.remove(self.data, 1)
        end
    end
    return f
end

test.gc.check_iterative = function()
    test.equal(gc.count(), 1)
    local fabric = create_fabric()
    test.equal(2, effil.gc.step())

    fabric:create(199)
    test.equal(gc.count(), 200)

    fabric:remove(50)
    collectgarbage()
    test.equal(gc.count(), 200)

    fabric:create(1) -- trigger GC
    test.equal(gc.count(), 151)
    fabric:remove(150)

    fabric:create(149)
    test.equal(gc.count(), 300)
    collectgarbage()

    fabric:create(1) -- trigger GC
    test.equal(gc.count(), 151)
end

test.gc.check_step = function()
    local fabric = create_fabric()
    effil.gc.step(3)

    fabric:create(299)
    fabric:remove(100)
    test.equal(gc.count(), 300)
    collectgarbage()

    fabric:create(1) -- trigger GC
    test.equal(gc.count(), 201)

    test.equal(3, effil.gc.step(2.5))

    fabric:create(299)
    fabric:remove(250)
    test.equal(gc.count(), 500)
    collectgarbage()

    fabric:create(1) -- trigger GC
    test.equal(gc.count(), 251)
end
