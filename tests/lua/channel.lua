TestChannels = {tearDown = tearDown}

function TestChannels:testCapacityUsage()
    local chan = effil.channel(2)

    test.assertTrue(chan:push(14))
    test.assertTrue(chan:push(88))
    test.assertFalse(chan:push(1488))

    test.assertEquals(chan:pop(), 14)
    test.assertEquals(chan:pop(), 88)
    test.assertIsNil(chan:pop(0))

    test.assertTrue(chan:push(14, 88), true)
    local ret1, ret2 = chan:pop()
    test.assertEquals(ret1, 14)
    test.assertEquals(ret2, 88)
end

function TestChannels:testRecursiveChannels()
    local chan1 = effil.channel()
    local chan2 = effil.channel()
    local msg1, msg2 = "first channel", "second channel"
    test.assertTrue(chan1:push(msg1, chan2))
    test.assertTrue(chan2:push(msg2, chan1))

    local ret1 = { chan1:pop() }
    test.assertEquals(ret1[1], msg1)
    test.assertEquals(type(ret1[2]), "userdata")
    local ret2 = { ret1[2]:pop() }
    test.assertEquals(ret2[1], msg2)
    test.assertEquals(type(ret2[2]), "userdata")
end

function TestChannels:testWithThread()
    local chan = effil.channel()
    local thread = effil.thread(function(chan)
            chan:push("message1")
            chan:push("message2")
            chan:push("message3")
            chan:push("message4")
        end
    )(chan)

    local start_time = os.time()
    test.assertEquals(chan:pop(),       "message1")
    thread:wait()
    test.assertEquals(chan:pop(0),      "message2")
    test.assertEquals(chan:pop(1),      "message3")
    test.assertEquals(chan:pop(1, 'm'), "message4")
    test.assertTrue(os.time() < start_time + 1)
end

function TestChannels:testWithSharedTable()
    local chan = effil.channel()
    local table = effil.table()

    local test_value = "i'm value"
    table.test_key = test_value

    chan:push(table)
    test.assertEquals(chan:pop().test_key, test_value)

    table.channel = chan
    table.channel:push(test_value)
    test.assertEquals(table.channel:pop(), test_value)
end

if WITH_EXTRA_CHECKS then

function TestChannels:testStressLoadWithMultipleThreads()
    local exchange_channel, result_channel = effil.channel(), effil.channel()

    local threads_number = 1000
    for i = 1, threads_number do
        effil.thread(function(exchange_channel, result_channel, indx)
                if indx % 2 == 0 then
                    for i = 1, 10000 do
                        exchange_channel:push(indx .. "_".. i)
                    end
                else
                    repeat
                        local ret = exchange_channel:pop(10)
                        if ret then
                            result_channel:push(ret)
                        end
                    until ret == nil
                end
            end
        )(exchange_channel, result_channel, i)
    end

    local data = {}
    for i = 1, (threads_number / 2) * 10000 do
        local ret = result_channel:pop(10)
        test.assertNotIsNil(ret)
        test.assertIsString(ret)
        test.assertIsNil(data[ret])
        data[ret] = true
    end

    for thr_id = 2, threads_number, 2 do
        for iter = 1, 10000 do
            test.assertTrue(data[thr_id .. "_".. iter])
        end
    end
end

function TestChannels:testTimedRead()
    local chan = effil.channel()
    local delayedWriter = function(channel, delay)
        require("effil").sleep(delay)
        channel:push("hello!")
    end
    effil.thread(delayedWriter)(chan, 70)

    local function check_time(real_time, use_time, metric, result)
        local start_time = os.time()
        test.assertEquals(chan:pop(use_time, metric), result)
        test.assertAlmostEquals(os.time(), start_time + real_time, 1)
    end
    check_time(2, 2, nil, nil) -- second by default
    check_time(2, 2, 's', nil)
    check_time(60, 1, 'm', nil)

    local start_time = os.time()
    test.assertEquals(chan:pop(10), "hello!")
    test.assertTrue(os.time() < start_time + 10)
end

end -- WITH_EXTRA_CHECKS
