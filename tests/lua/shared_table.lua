TestSharedTable = {tearDown = tearDown}

function TestSharedTable:testPairs()
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

function TestSharedTable:testSize()
    local share = effil.table()
    test.assertEquals(effil.size(share), 0)
    share[1] = 10
    test.assertEquals(effil.size(share), 1)
    share[2] = "value1"
    share["key1"] = function() end
    test.assertEquals(effil.size(share), 3)
    share[2] = nil
    test.assertEquals(effil.size(share), 2)
end

function TestSharedTable:testUserDataClassification()
    local share = effil.table()
    share.thread = effil.thread(function(a, b) return a + b end)(19, 33)
    share.sub_table = effil.table()
    share.sub_table.some_key = "some_value"

    local result = share.thread:get()
    test.assertEquals(result, 52)
    test.assertEquals(share.sub_table.some_key, "some_value")
end

TestGeneralSharedTableMetaTable = { tearDown = tearDown }

function TestGeneralSharedTableMetaTable:useMetatable(shared_table, metatable)
    local mt = self.test_param()
    for k, v in pairs(metatable) do
        mt[k] = v
    end
    effil.setmetatable(shared_table, mt)
end

function TestGeneralSharedTableMetaTable:testMetamethodIndex()
    local share = effil.table()
    self:useMetatable(share, {
            __index = function(t, key)
                return "mt_" .. effil.rawget(t, key)
            end
        }
    )
    share.table_key = "table_value"
    test.assertEquals(share.table_key, "mt_table_value")
end

function TestGeneralSharedTableMetaTable:testMetamethodNewIndex()
    local share = effil.table()
    self:useMetatable(share, {
            __newindex = function(t, key, value)
                effil.rawset(t, "mt_" .. key, "mt_" .. value)
            end
        }
    )
    share.table_key = "table_value"
    test.assertEquals(share.mt_table_key, "mt_table_value")
end

function TestGeneralSharedTableMetaTable:testMetamethodCall()
    local share = effil.table()
    self:useMetatable(share, {
            __call = function(t, val1, val2, val3)
                return tostring(val1) .. "_" .. tostring(val2), tostring(val2) .. "_" .. tostring(val3)
            end
        }
    )
    local first_ret, second_ret = share("val1", "val2", "val3")
    test.assertEquals(first_ret, "val1_val2")
    test.assertEquals(second_ret, "val2_val3")
end

local function CreateMetatableTestForBinaryOperator(method_info, op)
    TestGeneralSharedTableMetaTable["testMetamethod" .. (method_info.test_name and method_info.test_name or method_info.metamethod)] =
    function(self)
        local share = effil.table()
        self:useMetatable(share, {
                ['__' .. string.lower(method_info.metamethod)] = function(t, operand)
                    t.was_called = true
                    return t.value .. '_'.. tostring(operand)
                end
            }
        )
        share.was_called = false
        share.value = "left"
        test.assertEquals(op(share, method_info.operand or "right"),
                method_info.exp_value == nil and "left_right" or method_info.exp_value)
        test.assertTrue(share.was_called)
    end
end

local function CreateMetatableTestForUnaryOperator(methodName, op)
    TestGeneralSharedTableMetaTable["testMetamethod" .. methodName] =
    function(self)
        local share = effil.table()
        self:useMetatable(share, {
                ['__' .. string.lower(methodName)] = function(t)
                    t.was_called = true
                    return t.value .. "_suffix"
                end
            }
        )
        share.was_called = false
        share.value = "value"
        test.assertEquals(op(share), "value_suffix")
        test.assertTrue(share.was_called)
    end
end

CreateMetatableTestForBinaryOperator({ metamethod = "Concat"},  function(a, b) return a.. b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Add"},     function(a, b) return a + b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Sub"},     function(a, b) return a - b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Mul"},     function(a, b) return a * b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Div"},     function(a, b) return a / b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Mod"},     function(a, b) return a % b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Pow"},     function(a, b) return a ^ b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Le", exp_value = true }, function(a, b) return a <= b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Lt", exp_value = true }, function(a, b) return a < b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Eq", operand = effil.table(), exp_value = true },
        function(a, b) return a == b end)
CreateMetatableTestForBinaryOperator({ metamethod = "Eq", operand = effil.table(), exp_value = false, test_name = "Ne"},
        function(a, b) return a ~= b end)
--[[ TODO: Enable when the bug with SOL will be solved
    CreateMetatableTestForBinaryOperator({ metamethod = "Lt", test_name = "Gt"}, function(a, b) return a > b end, false)
    CreateMetatableTestForBinaryOperator({ metamethod = "Le", test_name = "Ge"}, function(a, b) return a >= b end, false)
]]

CreateMetatableTestForUnaryOperator("Unm",      function(a) return -a end)
CreateMetatableTestForUnaryOperator("ToString", function(a) return tostring(a) end)
CreateMetatableTestForUnaryOperator("Len",      function(a) return #a end)

make_test_with_param(TestGeneralSharedTableMetaTable, ".+" --[[ Any test in this test suite]],
        function() return {} end,
        function() return effil.table() end
)

TestSharedTableWithMetaTable = { tearDown = tearDown }

function TestSharedTableWithMetaTable:testMetamethodIterators()
    local share = effil.table()
    local iterator = self.test_param
    effil.setmetatable(share, {
            ["__" .. iterator] = function(table)
                return function(t, key)
                    local effil = require 'effil'
                    local ret = (key and key * 2) or 1
                    if ret > 2 ^ 10 then
                        return nil
                    end
                    return ret, effil.rawget(t, ret)
                end, table
            end
        }
    )
    -- Add some values
    for i = 0, 10 do
        local pow = 2 ^ i
        share[pow] = math.random(pow)
    end
    -- Add some noise
    for i = 1, 100 do
        share[math.random(1000) * 10 - 1] = math.random(1000)
    end
    -- Check that *pairs iterator works
    local pow_iter = 1
    for k,v in _G[iterator](share) do
        test.assertEquals(k, pow_iter)
        test.assertEquals(v, share[pow_iter])
        pow_iter = pow_iter * 2
    end
    test.assertEquals(pow_iter, 2 ^ 11)
end

make_test_with_param(TestSharedTableWithMetaTable, "testMetamethodIterators", "pairs", "ipairs")

function TestSharedTableWithMetaTable:testMetatableAsSharedTable()
    local share = effil.table()
    local mt = effil.table()
    effil.setmetatable(share, mt)
    -- Empty metatable
    test.assertEquals(share.table_key, nil)

    -- Only __index metamethod
    mt.__index = function(t, key)
        return "mt_" .. effil.rawget(t, key)
    end
    share.table_key = "table_value"
    test.assertEquals(share.table_key, "mt_table_value")

    -- Both __index and __newindex metamethods
    mt.__newindex = function(t, key, value)
        effil.rawset(t, key, "mt_" .. value)
    end
    share.table_key = "table_value"
    test.assertEquals(share.table_key, "mt_mt_table_value")

    -- Remove __index, use only __newindex metamethods
    mt.__index = nil
    share.table_key = "table_value"
    test.assertEquals(share.table_key, "mt_table_value")
end
