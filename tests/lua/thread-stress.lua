require "bootstrap-tests"

test.thread_stress.tear_down = default_tear_down

test.thread_stress.time = function ()
    local function check_time(real_time, use_time, metric)
        local start_time = os.time()
        effil.sleep(use_time, metric)
        test.almost_equal(os.time(), start_time + real_time, 1)
    end
    check_time(4, 4, nil) -- seconds by default
    check_time(4, 4, 's')
    check_time(4, 4000, 'ms')
    check_time(60, 1, 'm')
end
