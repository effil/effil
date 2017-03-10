#!/usr/bin/env lua

-- TODO: remove hardcode
package.path  = package.path .. ";../libs/luaunit/?.lua;../tests/lua/?.lua"

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

-----------
-- TESTS --
-----------

test = require "luaunit"
require 'test_utils'
require 'thread'
require 'shared_table'

os.exit( test.LuaUnit.run() )