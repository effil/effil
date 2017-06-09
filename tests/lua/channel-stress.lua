require "bootstrap-tests"

test.channel_stress.tear_down = default_tear_down

test.channel_stress.with_multiple_threads = function ()
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
        test.is_not_nil(ret)
        test.is_string(ret)
        test.is_nil(data[ret])
        data[ret] = true
    end

    for thr_id = 2, threads_number, 2 do
        for iter = 1, 10000 do
            test.is_true(data[thr_id .. "_".. iter])
        end
    end
end

test.channel_stress.timed_read = function ()
    local chan = effil.channel()
    local delayed_writer = function(channel, delay)
        require("effil").sleep(delay)
        channel:push("hello!")
    end
    effil.thread(delayed_writer)(chan, 70)

    local function check_time(real_time, use_time, metric, result)
        local start_time = os.time()
        test.equal(chan:pop(use_time, metric), result)
        test.almost_equal(os.time(), start_time + real_time, 1)
    end
    check_time(2, 2, nil, nil) -- second by default
    check_time(2, 2, 's', nil)
    check_time(60, 1, 'm', nil)

    local start_time = os.time()
    test.equal(chan:pop(10), "hello!")
    test.is_true(os.time() < start_time + 10)
end
