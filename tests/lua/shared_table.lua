TestSharedTable = {tearDown = tearDown}

function TestSharedTable:testPairs()
    local effil = require 'effil'
    local share = effil.table()
    local data = { 0, 0, 0, ["key1"] = 0, ["key2"] = 0, ["key3"] = 0 }

    for k, _ in pairs(data) do
        share[k] = k .. "-value"
    end

    for k,v in pairs(share) do
        test.assertEquals(data[k], 0)
        data[k] = 1
        test.assertEquals(v, k .. "-value")
    end

    for k,v in pairs(data) do
        log("Check: " .. k)
        test.assertEquals(v, 1)
    end

    for k,v in ipairs(share) do
        test.assertEquals(data[k], 1)
        data[k] = 2
        test.assertEquals(v, k .. "-value")
    end

    for k,v in ipairs(data) do
        log("Check: " .. k)
        test.assertEquals(v, 2)
    end
end

function TestSharedTable:testLength()
    local effil = require 'effil'
    local share = effil.table()
    share[1] = 10
    share[2] = 20
    share[3] = 30
    share[4] = 40
    share["str"] = 50
    log "Check values"
    test.assertEquals(#share, 4)
    share[3] = nil
    test.assertEquals(#share, 2)
    share[1] = nil
    test.assertEquals(#share, 0)
end
