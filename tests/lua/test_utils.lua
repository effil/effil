function log(...)
    local msg = '@\t\t' .. os.date('%Y-%m-%d %H:%M:%S ',os.time())
    for _, val in ipairs({...}) do
        msg = msg .. tostring(val) .. ' '
    end
    io.write(msg .. '\n')
    io.flush()
end

function wait(timeInSec, condition, silent)
    if not silent then
        log("Start waiting for " .. tostring(timeInSec) .. "sec...")
    end
    local result = false
    local startTime = os.time()
    while ( (os.time() - startTime) <= timeInSec) do
        if condition ~= nil then
            if type(condition) == 'function' then
                if condition() then
                    result = true
                    break
                end
            else
                if condition then
                    result = true
                    break
                end
            end
        end
    end
    if not silent then
        log "Give up"
    end
    return result
end

function sleep(timeInSec, silent)
    if not silent then
        log("Start sleep for " .. tostring(timeInSec) .. "sec...")
    end
    wait(timeInSec, nil, true)
    if not silent then
        log "Wake up"
    end
end

function tearDown()
    collectgarbage()
end

function make_test_with_param(test_suite, test_case_pattern, ...)
    local tests_to_delete, tests_to_add = {}, {}
    for test_name, test_func in pairs(test_suite) do
        if string.sub(test_name, 1, 4):lower() == 'test' and string.match(test_name, test_case_pattern) then
            table.insert(tests_to_delete, test_name)
            for i, param in ipairs({...}) do
                tests_to_add[test_name .. "/" .. i] = function(t)
                    print("#    with params: " .. tostring(param))
                    t.test_param = param
                    return test_func(t)
                end
            end
        end
    end
    for _, test_name in ipairs(tests_to_delete) do
        test_suite[test_name] = nil
    end
    for test_name, test_func in pairs(tests_to_add) do
        test_suite[test_name] = test_func
    end
end
