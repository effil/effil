require "bootstrap-tests"

local basic_type_mismatch_test_p = function(err_msg, wrong_arg_num, func_name , ...)
    local func_to_call = func_name
    if type(func_name) == "string" then
        func_to_call = effil
        for word in string.gmatch(func_name, "[^%.]+") do
            func_to_call = func_to_call[word]
            test.is_not_nil(func_to_call)
        end
    end

    local ret, err = pcall(func_to_call, ...)
    test.is_false(ret)
    print("Original error: '" .. err .. "'")

    -- because error may start with trace back
    local trunc_err = err
    if string.len(err) > string.len(err_msg) then
        trunc_err = string.sub(err, string.len(err) - string.len(err_msg) + 1, string.len(err))
    end
    test.equal(trunc_err, err_msg)
end

test.type_mismatch.input_types_mismatch_p = function(wrong_arg_num, expected_type, func_name, ...)
    local args = {...}
    local err_msg = "bad argument #" .. wrong_arg_num .. " to " ..
            (type(func_name) == "string" and "'effil." .. func_name or func_name.name) ..
            "' (" .. expected_type .. " expected, got " .. effil.type(args[wrong_arg_num]) .. ")"
    basic_type_mismatch_test_p(err_msg, wrong_arg_num, func_name, ...)
end

test.type_mismatch.unsupported_type_p = function(wrong_arg_num, func_name, ...)
    local args = {...}
    local err_msg = (type(func_name) == "string" and "effil." ..  func_name or func_name.name)
            .. ": unable to store object of " .. effil.type(args[wrong_arg_num]) .. " type"
    basic_type_mismatch_test_p(err_msg, wrong_arg_num, func_name, ...)
end

local function generate_tests()
    local function create_object_generator(name, func)
        return setmetatable({ name = name }, {
            __call = func,
            __tostring = function() return name end
        })
    end

    local channel_push_generator = create_object_generator("effil.channel:push",
        function(_, ...)
            return effil.channel():push(...)
        end
    )

    local thread_runner_generator = create_object_generator("effil.thread",
        function(_, ...)
            return effil.thread(function()end)(...)
        end
    )

    local table_set_value_generator = create_object_generator("effil.table",
        function(_, key, value)
            effil.table()[key] = value
        end
    )

    local table_get_value_generator = create_object_generator("effil.table",
        function(_, key)
            return effil.table()[key]
        end
    )

    local func = function()end
    local stable = effil.table()
    local thread = effil.thread(func)()
    thread:wait()
    local lua_thread = coroutine.create(func)

    local all_types = { 22, "s", true, {}, stable, func, thread, effil.channel(), lua_thread }

    for _, type_instance in ipairs(all_types) do
        local typename = effil.type(type_instance)

        -- effil.getmetatable
        if typename ~= "effil.table" then
            test.type_mismatch.input_types_mismatch_p(1, "effil.table", "getmetatable", type_instance)
        end

        -- effil.setmetatable
        if typename ~= "table" and typename ~= "effil.table" then
            test.type_mismatch.input_types_mismatch_p(1, "table", "setmetatable", type_instance, 44)
            test.type_mismatch.input_types_mismatch_p(2, "table", "setmetatable", {}, type_instance)
        end

        if typename ~= "effil.table" then
            -- effil.rawset
            test.type_mismatch.input_types_mismatch_p(1, "effil.table", "rawset", type_instance, 44, 22)
            -- effil.rawget
            test.type_mismatch.input_types_mismatch_p(1, "effil.table", "rawget", type_instance, 44)
            -- effil.ipairs
            test.type_mismatch.input_types_mismatch_p(1, "effil.table", "ipairs", type_instance)
            -- effil.pairs
            test.type_mismatch.input_types_mismatch_p(1, "effil.table", "pairs", type_instance)
        end

        -- effil.thread
        if typename ~= "function" then
            test.type_mismatch.input_types_mismatch_p(1, "function", "thread", type_instance)
        end

        -- effil.sleep
        if typename ~= "number" then
            test.type_mismatch.input_types_mismatch_p(1, "number", "sleep", type_instance, "s")
        end
        if typename ~= "string" then
            test.type_mismatch.input_types_mismatch_p(2, "string", "sleep", 1, type_instance)
        end

        if typename ~= "number" then
            -- effil.channel
            test.type_mismatch.input_types_mismatch_p(1, "number", "channel", type_instance)

            --  effil.gc.step
            test.type_mismatch.input_types_mismatch_p(1, "number", "gc.step", type_instance)
        end

        -- effil.dump
        if typename ~= "table" and typename ~= "effil.table" then
            test.type_mismatch.input_types_mismatch_p(1, "table", "dump", type_instance)
        end
    end

    -- Below presented tests which support everything except coroutines

    -- effil.rawset
    test.type_mismatch.unsupported_type_p(2, "rawset", stable, lua_thread, 22)
    test.type_mismatch.unsupported_type_p(3, "rawset", stable, 44, lua_thread)

    -- effil.rawget
    test.type_mismatch.unsupported_type_p(2, "rawget", stable, lua_thread)

    -- effil.channel:push()
    test.type_mismatch.unsupported_type_p(1, channel_push_generator, lua_thread)

    -- effil.thread()()
    test.type_mismatch.unsupported_type_p(1, thread_runner_generator, lua_thread)

    -- effil.table[key] = value
    test.type_mismatch.unsupported_type_p(1, table_set_value_generator, lua_thread, 2)
    test.type_mismatch.unsupported_type_p(2, table_set_value_generator, 2, lua_thread)
    -- effil.table[key]
    test.type_mismatch.unsupported_type_p(1, table_get_value_generator, lua_thread)
end

-- Put it to function to limit the lifetime of objects
generate_tests()

test.type_mismatch.gc_checks_after_tests = function()
    default_tear_down()
end