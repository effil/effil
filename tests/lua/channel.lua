TestChannels = {tearDown = tearDown}

function TestChannels:testCapacityUsage()
    local chan = effil.channel(2)
    test.assertEquals(chan:write(14), true)
    test.assertEquals(chan:write(88), true)
    test.assertEquals(chan:write(1488), false)
    test.assertEquals(chan:read(), 14)
    test.assertEquals(chan:read(), 88)
    test.assertEquals(chan:read(0), nil)
end

function TestChannels:testWithThread()
    local chan = effil.channel()
    effil.thread(function(chan)
            chan:write("message1")
            chan:write("message2")
            chan:write("message3")
            chan:write("message4")
        end
    )(chan)

    local start_time = os.time()
    test.assertEquals(chan:read(),       "message1")
    test.assertEquals(chan:read(0),      "message2")
    test.assertEquals(chan:read(1),      "message3")
    test.assertEquals(chan:read(1, 'm'), "message4")
    test.assertTrue(os.time() < start_time + 1)
end

function TestChannels:testWithSharedTable()
    local chan = effil.channel()
    local table = effil.table()

    local test_value = "i'm value"
    table.test_key = test_value

    chan:write(table)
    test.assertEquals(chan:read().test_key, test_value)

    table.channel = chan
    table.channel:write(test_value)
    test.assertEquals(table.channel:read(), test_value)
end

if WITH_EXTRA_CHECKS then

function TestChannels:testStressLoadWithMultipleThreads()
    local exchange_channel, result_channel = effil.channel(), effil.channel()

    local threads_number = 1000
    for i = 1, threads_number do
        effil.thread(function(exchange_channel, result_channel, indx)
                if indx % 2 == 0 then
                    for i = 1, 10000 do
                        exchange_channel:write(indx .. "_".. i)
                    end
                else
                    repeat
                        local ret = exchange_channel:read(10)
                        if ret then
                            result_channel:write(ret)
                        end
                    until ret == nil
                end
            end
        )(exchange_channel, result_channel, i)
    end

    local data = {}
    for i = 1, (threads_number / 2) * 10000 do
        local ret = result_channel:read(10)
        test.assertNotIsNil(ret)
        test.assertIsString(ret)
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
        channel:write("hello!")
    end
    effil.thread(delayedWriter)(chan, 70)

    local function check_time(real_time, use_time, metric, result)
        local start_time = os.time()
        test.assertEquals(chan:read(use_time, metric), result)
        test.assertAlmostEquals(os.time(), start_time + real_time, 1)
    end
    check_time(2, 2, nil, nil) -- second by default
    check_time(2, 2, 's', nil)
    check_time(60, 1, 'm', nil)

    local start_time = os.time()
    test.assertEquals(chan:read(10), "hello!")
    test.assertTrue(os.time() < start_time + 7)
end

end -- WITH_EXTRA_CHECKS
