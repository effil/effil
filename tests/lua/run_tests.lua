#!/usr/bin/env lua

-- TODO: remove hardcode
package.path  = package.path .. ";../libs/luaunit/?.lua;../tests/lua/?.lua"

print("---------------")
print("--  " .. _VERSION .. "  --")
print("---------------")

do
    -- Hack input arguments to make tests verbose by default
    local make_verbose = true
    for i, v in ipairs(arg) do
        if v == '-o' or v == '--output' then
            make_verbose = false
        elseif v == "--extra-checks" then
            table.remove(arg, i)
            WITH_EXTRA_CHECKS = true
            print "# RUN TESTS WITH EXTRA CHECKS"
        end
    end
    if make_verbose then
        table.insert(arg, '-o')
        table.insert(arg, 'TAP')
    end
end

-----------
-- TESTS --
-----------

test = require "luaunit"
effil = require 'effil'
require 'test_utils'
require 'thread'
require 'shared_table'
require 'channel'

-- Hack tests functions to print when test starts
for suite_name, suite in pairs(_G) do
    if string.sub(suite_name, 1, 4):lower() == 'test' and type(_G[suite_name]) == 'table' then -- is a test suite
        for test_name, test_func in pairs(suite) do
            if string.sub(test_name, 1, 4):lower() == 'test' then -- is a test function
                suite[test_name] = function(...)
                    print("#  Starting test: " .. suite_name .. '.' .. test_name)
                    return test_func(...)
                end
            end
        end
    end
end

os.exit( test.LuaUnit.run() )