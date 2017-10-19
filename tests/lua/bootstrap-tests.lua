effil = require "effil"
test = require "u-test"

local major, minor = _VERSION:match("Lua (%d).(%d)")
LUA_VERSION = major * 10 + minor

function default_tear_down()
    collectgarbage()
    effil.gc.collect()
    -- effil.G is always present
    -- thus, gc has one object
    if effil.gc.count() ~= 1 then
        print "Not all bojects were removed, gonna sleep for 2 seconds"
        effil.sleep(2)

        collectgarbage()
        effil.gc.collect()
    end
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


if not table.unpack then
    table.unpack = unpack
end
