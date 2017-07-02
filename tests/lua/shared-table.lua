require "bootstrap-tests"

test.shared_table.tear_down = default_tear_down

test.shared_table.constructor = function ()
    local share = effil.table {
        key = "value"
    }
    test.equal(share.key, "value")
    test.equal(pcall(effil.table, ""), false)
    test.equal(pcall(effil.table, 22), false)
    test.equal(pcall(effil.table, effil.table()), false)
end

if LUA_VERSION > 51 then

test.shared_table.pairs = function ()
    local share = effil.table()
    local data = { 0, 0, 0, ["key1"] = 0, ["key2"] = 0, ["key3"] = 0 }

    for k, _ in pairs(data) do
        share[k] = k .. "-value"
    end

    for k,v in pairs(share) do
        test.equal(data[k], 0)
        data[k] = 1
        test.equal(v, k .. "-value")
    end

    for k,v in pairs(data) do
        test.equal(v, 1)
    end

    for k,v in ipairs(share) do
        test.equal(data[k], 1)
        data[k] = 2
        test.equal(v, k .. "-value")
    end

    for k,v in ipairs(data) do
        test.equal(v, 2)
    end
end

end -- LUA_VERSION > 51

test.shared_table.length = function ()
    local share = effil.table()
    share[1] = 10
    share[2] = 20
    share[3] = 30
    share[4] = 40
    share["str"] = 50
    test.equal(#share, 4)
    share[3] = nil
    test.equal(#share, 2)
    share[1] = nil
    test.equal(#share, 0)
end

test.shared_table.size = function ()
    local share = effil.table()
    test.equal(effil.size(share), 0)
    share[1] = 10
    test.equal(effil.size(share), 1)
    share[2] = "value1"
    share["key1"] = function() end
    test.equal(effil.size(share), 3)
    share[2] = nil
    test.equal(effil.size(share), 2)
end

test.shared_table.user_data_classification = function ()
    local share = effil.table()
    share.thread = effil.thread(function(a, b) return a + b end)(19, 33)
    share.sub_table = effil.table()
    share.sub_table.some_key = "some_value"

    local result = share.thread:get()
    test.equal(result, 52)
    test.equal(share.sub_table.some_key, "some_value")
end

test.shared_table.global = function ()
    test.not_equal(effil.G, nil)
    effil.G.test_key = "test_value"
    local thr = effil.thread(function()
        local effil = require "effil"
        if effil.G == nil or effil.G.test_key ~= "test_value" then
            error("Invalid value of global table: " .. tostring(effil.G and effil.G.test_key or nil))
        end
        effil.G.test_key = "checked"
    end)()
    local status, err = thr:wait()
    if status == "failed" then
        print("Thread failed with message: " .. err)
    end
    test.equal(status, "completed")
    test.equal(effil.G.test_key, "checked")
end
