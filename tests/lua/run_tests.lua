#!/usr/bin/env lua

-- TODO: remove hardcode
package.path  = package.path .. ";../libs/luaunit/?.lua"
package.cpath = package.cpath .. ";./?.so;./?.dylib"

test = require "luaunit"

do
    -- Hack input arguments to make tests verbose by default
    local found = false
    for _, v in ipairs(arg) do
        if v == '-o' or v == '--output' then
            found = true
            break
        end
    end
    if not found then
        table.insert(arg, '-o')
        table.insert(arg, 'TAP')
    end
end

function log(...)
    local msg = '@\t\t' .. os.date('%Y-%m-%d %H:%M:%S ',os.time())
    for _, val in ipairs({...}) do
        msg = msg .. tostring(val) .. ' '
    end
    io.write(msg .. '\n')
    io.flush()
end

function wait(timeInSec, condition)
    local startTime = os.time()
    while ( (os.time() - startTime) <= timeInSec) do
        if condition ~= nil then
            if type(condition) == 'function' then
                if condition() then
                    return true
                end
            else
                if condition then
                    return true
                end
            end
        end
    end
    return false
end

function sleep(timeInSec)
    wait(timeInMsec, nil)
end

-----------
-- TESTS --
-----------

require("smoke_test")

-----------
os.exit( test.LuaUnit.run() )