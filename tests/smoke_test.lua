local thr = require('woofer')
t = thr.new(
    function()
        local thr = require('woofer')
        print(share['key1'])
        print(share['key2'])
        print(share['key3'])
        print(thr.thread_id())
        for i = 1, 100 do
            io.write('2')
            io.flush()
            os.execute("sleep 0.1")
        end
    end
)

share['key1'] = 'val'
share['key2'] = 100500
share['key3'] = true
print(thr.thread_id())
for i = 1, 100 do
    io.write('1')
    io.flush()
    os.execute("sleep 0.1")
end
t:join()

--[[
ppp("qwe")
t = thr.new(
    function()
        local thr = require('bevy')
        print(("0x%x"):format(thr.thread_id()))
        for i = 1, 10 do
            io.write('.')
            io.flush()
            os.execute("sleep " .. 1)
        end
    end
)
print(("0x%x"):format(thr.thread_id()))

for i = 1, 10 do
    io.write(',')
    io.flush()
    os.execute("sleep " .. 1)
end

t:join()

print()
]]

--[[
str_foo = ""

asd =10
qwe = 20
function bar()
    local lala = { 40 }
    function foo(a, b)
        local d = asd
        local l = lala[1]
        lala = 10
        return function() return a + asd + b + qwe + l end
    end

    print(debug.getupvalue(foo, 1))
    print(debug.getupvalue(foo, 2))
    table_print(debug.getinfo(foo))
    str_foo = string.dump(foo)
end

bar()

foo2 = loadstring(str_foo)


local t = {}
setmetatable(t,
    {
        __index = function(self, key)
            print("Call ", key)
            return _ENV[key]
        end,
        __newindex = function(self, key, value)
            print("Call ", key, value)
            _ENV[key] = value
        end
    }
)
debug.setupvalue(foo2, 1, t)
debug.setupvalue(foo2, 2, { 99 })
print(foo2(10, 20)())
]]
