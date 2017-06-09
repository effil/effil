effil = require "effil"
test = require "u-test"

function default_tear_down()
    collectgarbage()
    effil.gc.collect()
    -- effil.G is always present
    -- thus, gc has one object
    test.equal(effil.gc.count(), 1)
end

function wait(timeInSec, condition, silent)
    local result = false
    local startTime = os.time()
    while ( (os.time() - startTime) <= timeInSec) do
        if condition ~= nil then
            if type(condition) == 'function' then
                if condition() then
                    result = true
                    break
                end
            else
                if condition then
                    result = true
                    break
                end
            end
        end
    end
    return result
end


function sleep(timeInSec, silent)
    wait(timeInSec, nil, true)
end
