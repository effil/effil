require "bootstrap-tests"

test.size = function()
    local st = effil.table()
    st[0] = 1
    st[1] = 0

    local chan = effil.channel()
    chan:push(0)
    chan:push(2)

    test.equal(effil.size(st), 2)
    test.equal(effil.size(chan), 2)
end
