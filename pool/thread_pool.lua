local effil = require "effil"
local heap = require "heap"

local pool_api = {}

local function thread_worker(context)
    local effil = require "effil"
    local req  = context.req_chan
    local resp = context.resp_chan

    print("thread started")
    local curr_task = nil
    while true do
        if curr_task then
            coroutine.resume(curr_task.co, effil.unpack(curr_task.context.args))
            if coroutine.status(curr_task.co) == "dead" then
                curr_task = nil
                context.curr_prior = nil
            end
        else
            local t = resp:pop(0)
            if t == nil then
                effil.sleep(100, "ms")
            elseif effil.type(t) == "string" then
                if t == "shutdown" then
                    print("thread stoped")
                    return
                end
            elseif effil.type(t) == "effil.table" then
                assert(type(t.func) == "function")--, effil.type(t.func))
                curr_task = {
                    context = t,
                    co = coroutine.create(t.func)
                }
                context.curr_prior = t.prior
            else
                error("Thread worker got invalid response: "
                    .. tostring(t) .. " (" .. effil.type(t) .. ")")
            end
        end
    end
end

local function run_threads(pool, num)
    local effil = require "effil"

    for i = 1, num do
        local context = effil.table {
            req_chan = effil.channel(),
            resp_chan = effil.channel()
        }
        local handle = effil.thread(thread_worker)(context)

        local thr = effil.table {
            context = context,
            handle = handle
        }
        table.insert(pool, thr)
    end
end

local function main_thread(cmd_chan, threads_num, queue_size)
    local heap = require "heap"
    local effil = require "effil"

    local threads = {}
    run_threads(threads, threads_num)
    assert(#threads == threads_num)

    local tasks = heap:new(function(a,b) return a > b end)

    while true do
        local cmd = cmd_chan:pop(0)
        while cmd do
            if cmd == "shutdown" then
                for _, t in ipairs(threads) do
                    t.context.resp_chan:push("shutdown")
                    t.handle:wait()
                end
                return
            elseif effil.type(cmd) == "effil.table" and tasks.length < queue_size then
                tasks:insert(cmd.prior, cmd)
            end
            cmd = cmd_chan:pop(0)
        end

        if not tasks:empty() then
            local _, task = tasks:pop()
            while task ~= nil do
                for i, thr in ipairs(threads) do
                    if thr.handle:status() ~= "running" then
                        print(i .. ":", thr.handle:status())
                    end
                    if thr.context.curr_prior == nil then
                        thr.context.resp_chan:push(task)
                        task = nil
                        if not tasks:empty() then
                            _, task = tasks:pop()
                        end
                    end
                end
                if task ~= nil then
                    tasks:insert(task.prior, task)
                    break
                end
            end
        end
    end
end

function pool_api.create()
    local pool = effil.table {
        cmd_chan = effil.channel()
    }

    pool.start = function(self, threads_num, queue_size)
        self.main_thread = require("effil").thread(main_thread)
            (self.cmd_chan, threads_num, queue_size)
    end

    pool.stop = function(self, ...)
        self.cmd_chan:push("shutdown")
        local ret = self.main_thread:wait(...)
        return ret == "completed"
    end

    pool.add_task = function(self, prior, task, ...)
        if self.main_thread and self.main_thread:status() ~= "running" then
            print(i .. ":", self.main_thread:status())
        end
        self.cmd_chan:push(require("effil").table
            { prior = prior, func = task, args = {...} })
    end

    return pool
end

-- TEST

local pool = pool_api.create()

local ch = effil.channel()
pool:add_task(1, function() require("effil").sleep(2) ch:push(1) end)
pool:add_task(2, function() require("effil").sleep(2) ch:push(2) end)
pool:add_task(3, function() require("effil").sleep(2) ch:push(3) end)
pool:add_task(4, function() require("effil").sleep(2) ch:push(4) end)
pool:add_task(5, function() require("effil").sleep(2) ch:push(5) end)

pool:start(5, 10)

while true do
    local msg = ch:pop(0)
    if msg then
        print("Test got:" .. msg)
    end
    local prior = math.floor(math.random() * 1000)
    pool:add_task(prior, function(p) ch:push(p) end, prior)
end

print(pool:stop())
