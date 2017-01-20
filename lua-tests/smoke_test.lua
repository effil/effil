TestSmoke = {}

function TestSmoke:testGeneralWorkability()
    local woofer = require('libwoofer')
    local share = woofer.share()

    share["number"] = 100500
    share["string"] = "string value"
    share["bool"] = true

    log "Start thread"
    local thread = woofer.thread(
        function(share)
            share["child.number"] = share["number"]
            share["child.string"] = share["string"]
            share["child.bool"]   = share["bool"]
        end,
        share
    )
    log "Join thread"
    thread:join()

    log "Check values"
    test.assertEquals(share["child.number"], share["number"],
        "'number' fields are not equal")
    test.assertEquals(share["child.string"], share["string"],
        "'string' fields are not equal")
    test.assertEquals(share["child.bool"], share["bool"],
        "'bool' fields are not equal")
end

function TestSmoke:testDetach()
    local woofer = require('libwoofer')
    local share = woofer.share()

    share["finished"] = false
    log "Start thread"
    local thread = woofer.thread(
        function(share)
            local startTime = os.time()
            while ( (os.time() - startTime) <= 3) do --[[ Like we are working 3sec ... ]] end
            share["finished"] = true
        end,
        share
    )
    log "Detach thread"
    thread:detach()

    log "Waiting for thread completion..."
    test.assertEquals(wait(4, function() return share["finished"] end) , true)
    log "Stop waiting"
end
