#!/usr/bin/env lua

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

-- require "channel_stress"
test.summary()