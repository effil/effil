require "bootstrap-tests"

test.channel.tear_down = default_tear_down

test.channel.capacity_usage = function()
    local chan = effil.channel(2)

    test.is_true(chan:push(14))
    test.is_true(chan:push(88))
    test.equal(chan:size(), 2)

    test.is_false(chan:push(1488))

    test.equal(chan:pop(), 14)
    test.equal(chan:pop(), 88)
    test.is_nil(chan:pop(0))
    test.equal(chan:size(), 0)

    test.is_true(chan:push(14, 88), true)
    local ret1, ret2 = chan:pop()
    test.equal(ret1, 14)
    test.equal(ret2, 88)
end

test.channel.recursive = function ()
    local chan1 = effil.channel()
    local chan2 = effil.channel()
    local msg1, msg2 = "first channel", "second channel"
    test.is_true(chan1:push(msg1, chan2))
    test.is_true(chan2:push(msg2, chan1))

    local ret1 = { chan1:pop() }
    test.equal(ret1[1], msg1)
    test.equal(type(ret1[2]), "userdata")
    local ret2 = { ret1[2]:pop() }
    test.equal(ret2[1], msg2)
    test.equal(type(ret2[2]), "userdata")
end

test.channel.with_threads = function ()
    local chan = effil.channel()
    local thread = effil.thread(function(chan)
            chan:push("message1")
            chan:push("message2")
            chan:push("message3")
            chan:push("message4")
        end
    )(chan)

    local start_time = os.time()
    test.equal(chan:pop(),       "message1")
    thread:wait()
    test.equal(chan:pop(0),      "message2")
    test.equal(chan:pop(1),      "message3")
    test.equal(chan:pop(1, 'm'), "message4")
    test.is_true(os.time() < start_time + 1)
end

test.channel.with_shared_table = function ()
    local chan = effil.channel()
    local table = effil.table()

    local test_value = "i'm value"
    table.test_key = test_value

    chan:push(table)
    test.equal(chan:pop().test_key, test_value)

    table.channel = chan
    table.channel:push(test_value)
    test.equal(table.channel:pop(), test_value)
end
