require "bootstrap-tests"

local effil = require "effil"

test.channel_stress.tear_down = default_tear_down

test.channel_stress.with_multiple_threads = function ()
    local exchange_channel, result_channel = effil.channel(), effil.channel()

    local threads_number = 20 * tonumber(os.getenv("STRESS"))
    local threads = {}
    for i = 1, threads_number do
        threads[i] = effil.thread(function(exchange_channel, result_channel, indx)
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

    for _, thread in ipairs(threads) do
        thread:wait()
    end
end

-- TODO: fix it for Windows
if not os.getenv("APPVEYOR") then
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
end

-- regress for channel returns
test.channel_stress.retun_tables = function ()
    local function worker()
        local effil = require "effil"
        local ch = effil.channel()
        for i = 1, 1000 do
            ch:push(effil.table())
            local ret = { ch:pop() }
        end
    end

    local threads = {}

    for i = 1, 20 do
        table.insert(threads, effil.thread(worker)())
    end
    for _, thr in ipairs(threads) do
        thr:wait()
    end
end

-- regress for multiple wait on channel
test.channel_stress.regress_for_multiple_waiters = function ()
    for i = 1, 20 do
        local chan = effil.channel()
        local function receiver()
            return chan:pop(5) ~= nil
        end

        local threads = {}
        for i = 1, 10 do
            table.insert(threads, effil.thread(receiver)())
        end

        effil.sleep(0.1)
        for i = 1, 100 do
            chan:push(1)
        end

        for _, thr in ipairs(threads) do
            local ret = thr:get()
            test.is_true(ret)
            if not ret then
                return
            end
        end
    end
end
