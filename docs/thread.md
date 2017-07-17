# effil.thread
==============
## Overview
`effil.thread` is a way to create thread. Threads can be stopped, paused, resumed and canceled.
All operation with threads can be synchronous (with or without timeout) or asynchronous. 
Each thread runs with its own lua state.
**Do not run function with upvalues in `effil.thread`**  
Use `effil.table` and `effil.channel` to transmit data over threads.

## Example
```lua
local function greeting(name)
    return "Hello, " .. name
end

local effil = require "effil"
local runner = effil.thread(greeting) -- capture function to thread runner
local thr1 = runner("Sparky") -- create first thread
local thr2 = runner() -- create second thread

print(thr1:get()) -- get result
print(thr2:get()) -- get result
```

## API
`runner = effil.threas(f)` - creates thread runner. Runner spawn new thread for each invocation.  
### Thread runner
- `runner.path` - `package.path` value for new state. Default value inherits `package.path` form parent state.
- `runner.path` - `package.cpath` value for new state. Default value inherits `package.cpath` form parent state.
- `runner.step` - number of lua instructions lua between cancelation points. Default value is 200. Spawn unstopable threads when value is equal to 0.    
- `thr = runner(arg1, arg2, arg3)` - run captured function with this args in separate thread and returns handle.

### Thread handle
Thread handle provides API for interation with shild thread.
- You can use `effil.table` and `effil.channel` share this handles between threads.
- You can call any handle methods from multiple threads.
- You don't need to save this handle if you do not want to communicate with thread. 
 

All functions:
- `thr:status()` - return thread status. Possible values are: `"running", "paused", "canceled", "completed" and "failed"`
- `thr:get(time, metric)` - waits for thread completion and returns function result or `nil` in case of error.
- `thr:wait(time, metric)` - waits for thread completion and returns thread status with error message is `"failed"`.
- `thr:cancel(time, metric)` - interrupt thread execution.
- `thr:pause(time, metric)` - pause thread.
- `thr:resume(time, metric)` - resume thred.

All operations can be bloking or non blocking.
- `thr:get()` - blocking wait for thread completion.
- `thr:get(0)` - non blocking get.
- `thr:get(50, "ms")` - blocking wait for 50 milliseconds.
- `the:get(1)` - blocking wait for 1 second.

Metrics:
- `ms` - milliseconds;
- `s` - seconds;
- `m` - minutes;
- `h` - hours.

### Thread helpers
`effil.thread_id()` - unique string thread id. 
`effil.yield()` - explicit cancellation point.
`effil.sleep(time, metric)` - suspend current thread. `metric` is optional and default is seconds.
