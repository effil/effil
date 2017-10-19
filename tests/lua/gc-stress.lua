require "bootstrap-tests"

test.gc_stress.tear_down = default_tear_down

-- Regress test for simultaneous object creation and removing
test.gc_stress.create_and_collect_in_parallel = function ()
    function worker()
        effil = require "effil"
        local nested_table = {
                {}, --[[1 level]]
                {{}}, --[[2 levels]]
                {{{}}}, --[[3 levels]]
                {{{{}}}} --[[4 levels]]
        }
        for i = 1, 20 * tonumber(os.getenv("STRESS")) do
            for t = 1, 10 do
                local tbl = effil.table(nested_table)
                for l = 1, 10 do
                    tbl[l] = nested_table
                end
            end
            collectgarbage()
            effil.gc.collect()
        end
    end

    local thread_num = 10
    local threads = {}

    for i = 1, thread_num do
        threads[i] = effil.thread(worker)(i)
    end

    for i = 1, thread_num do
        test.equal(threads[i]:wait(), "completed")
    end
end
