require "bootstrap-tests"

local function table_included(left, right, path)
    local path = path or ""
    if type(left) ~= type(right) then
        return false, "[" .. path .. "]: " .." got " .. type(right) .. "instead of " .. type(left)
    end

    for k, v in pairs(left) do
        local subpath = path .. '.' .. tostring(k)
        if type(v) == 'table' then
            local ret, msg = table_included(v, right[k], subpath)
            if not ret then
                return false, msg
            end
        elseif right[k] ~= v then
            return false, "[" .. subpath .. "]: got " .. tostring(right[k]) .. " instead of " .. tostring(v)
        end
    end
    return true
end

local function table_equals(left, right)
    local ret, msg = table_included(left, right)
    if not ret then
        return false, msg
    end
    return table_included(right, left)
end

test.dump_table.tear_down = default_tear_down

test.dump_table.compare_primitives = function()
    local origin = {
        1, "str", key = "value",
        key2 = { 2, [false] = "asd", { [44] = {true} } }
    }

    local result = effil.dump(effil.table(origin))
    assert(table_equals(origin, result))
end

test.dump_table.compare_functions = function()
    local origin = {
        func = function(a, b) return a + b end,
        nested = {
            [function(a, b) return a - b end] = 2
        },
    }

    local result = effil.dump(effil.table(origin))
    test.equal(origin.func(2, 53), result.func(2, 53))
    for origin_key, origin_value in pairs(origin.nested) do
        for res_key, res_value in pairs(result.nested) do
            test.equal(origin_key(23, 11), res_key(23, 11))
            test.equal(origin_value, res_value)
        end
    end
end

test.dump_table.reference_loop = function()
    local origin = {}
    origin.nested = {1, origin, 2}
    origin.nested.nested_loop = { [origin] = origin.nested }

    local result = effil.dump(effil.table(origin))
    test.equal(result.nested[1], 1)
    test.equal(result.nested[2], result)
    test.equal(result.nested[3], 2)
    test.equal(result.nested.nested_loop[result], result.nested)
end

test.dump_table.regular_table = function()
    local origin = {}
    test.equal(origin, effil.dump(origin))
end

test.dump_table.upvalues_with_loop = function()
    local origin = {}
    local function foo()
        origin.key = "value"
    end
    origin.foo = foo

    local result = effil.dump(origin)
    local name, value = debug.getupvalue(result.foo, 1)
    test.equal(value, result)
    result.foo()
    test.equal(result.key, "value")
end
