#!/usr/bin/env lua

package.path = ";../tests/lua/?.lua;../libs/u-test/?.lua;../src/lua/?.lua"

require "type"
require "gc"
require "channel"
require "thread"
require "shared-table"
require "metatable"

if os.getenv("STRESS") then
    require "channel-stress"
    require "thread-stress"
end

test.summary()