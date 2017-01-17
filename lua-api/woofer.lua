--local thr = require('libbevy')
require('libbevy')
local thr = thread
return {
    new = function(func)
        local str_func = ("%q"):format(string.dump(func))
        return thr.new(str_func)
    end,
    thread_id = thr.thread_id
}
