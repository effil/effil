TestChannels = {tearDown = tearDown}

function TestChannels:testCapacityUsage()
    local chan = effil.channel(2)
    test.assertEquals(chan:write(14), true)
    test.assertEquals(chan:write(88), true)
    test.assertEquals(chan:write(1488), false)
    test.assertEquals(chan:read(), 14)
    --[[
    test.assertEquals(chan:read(), 88)
    ]]
end
