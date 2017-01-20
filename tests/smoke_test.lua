function compare(o1, o2)
    if o1 == o2 then return true end
    local o1Type = type(o1)
    local o2Type = type(o2)
    if o1Type ~= o2Type then return false end
    if o1Type ~= 'table' then return false end

    local keySet = {}
    for key1, value1 in pairs(o1) do
        local value2 = o2[key1]
        if value2 == nil or equals(value1, value2) == false then
            return false
        end
        keySet[key1] = true
    end

    for key2, _ in pairs(o2) do
        if not keySet[key2] then return false end
    end
    return true
end

function check(left, right)
    if not compare(left, right) then
        print("ERROR")
    end
end

-----------
-- TESTS --
-----------

do -- Simple smoke
    package.cpath = package.cpath .. ";./?.dylib" -- MAC OS support
    local woofer = require('libwoofer')
    local share = woofer.share()

    share["number"] = 100500
    share["string"] = "string value"
    share["bool"] = true

    local thread = woofer.thread(
        function(share)
            share["child.number"] = share["number"]
            share["child.string"] = share["string"]
            share["child.bool"]   = share["bool"]
        end,
        share
    )
    thread:join()

    check(share["child.number"], share["number"])
    check(share["child.string"], share["string"])
    check(share["child.bool"], share["bool"])
end
