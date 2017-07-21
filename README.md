# Effil
[![Build Status](https://travis-ci.org/loud-hound/effil.svg?branch=master)](https://travis-ci.org/loud-hound/effil)

Effil is a lua module for multithreading support.
It allows to spawn native threads and safe data exchange.
Effil has been designed to provide clear and simple API for lua developers including threads, channels and shared tables.

Effil supports lua 5.1, 5.2, 5.3 and LuaJIT.
Requires C++14 compiler compliance. Tested with GCC 5, clang 3.8 and Visual Studio 2015.

## How to install
### Build from src on Linux and Mac
1. `git clone git@github.com:loud-hound/effil.git effil && cd effil`
2. `mkdir build && cd build && make -j4 install`
3. Copy effil.lua and libeffil.so/libeffil.dylib to your project.

### From lua rocks
Coming soon.

## Quick guide for impatient
As you may now there are not much script
languages with **real** multithreading support
(Lua/Python/Ruby and etc has global interpreter lock aka GIL).
Effil solves this problem by running independent Lua VM
in separate native thread and provides robust communicating primitives
for creating threads (VM instances) and data sharing.

Effil library provides two major functions:
1. `effil.thread(action)` - function which creates threads.
2. `effil.table` - table that persist in all threads and behaves just like regular lua table.
3. Bunch og utilities to handle threads and tables.

## Examples
### Spawn the thread
```lua
local effil = require("effil")

function bark(name)
    print(name .. " barks from another thread!")
end

-- run funtion bark in separate thread with name "Spaky"
local thr = effil.thread(bark)("Sparky")

-- wait for completion
thr:wait()
```
**Output:**
`Sparky barks from another thread!`

### Shareing data with effil.channel
```lua
local effil = require("effil")

-- channel allow to push date in one thread and pop in other
local channel = effil.channel()

-- writes some numbers to channel
local function producer(channel)
    for i = 1, 5 do
        print("push " .. i)
        channel:push(i)
    end
    channel:push(nil)
end

-- read numbers from channels
local function consumer(channel)
    local i = channel:pop()
    while i do
        print("pop " .. i)
        i = channel:pop()
    end
end

-- run producer
local thr = effil.thread(producer)(channel)

-- run consumer
consumer(channel)

thr:wait()
```
**Output:**
```
push 1
push 2
pop 1
pop 2
push 3
push 4
push 5
pop 3 
pop 4
pop 5
```

### Sharing data with effil.table
```lua
local effil = require("effil")

-- effil.table transfers data between threads
-- and behaves like regualr lua table
local storage = effil.table {}

function download_file(storage, url)
    local restult =  {}
    restult.downloaded = true
    restult.content = "I am form " .. url
    restult.bytes = #restult.content
    storage[url] = restult
end

-- capture download function
local downloader = effil.thread(download_file)
local downloads = effil.table {}
for _, url in pairs({"luarocks.org", "ya.ru", "github.com" }) do
    -- run downloads in separate threads
    -- each invocation creates separate thread
    downloads[#downloads + 1] = downloader(storage, url)
end

for _, download in pairs(downloads) do
    download:wait()
end

for url, result in pairs(storage) do
    print('From ' .. url .. ' downloaded ' ..
            result.bytes .. ' bytes, content: "' .. result.content  .. '"')
end
```
**Output:**
```
From github.com downloaded 20 bytes, content: "I am form github.com"
From luarocks.org downloaded 22 bytes, content: "I am form luarocks.org"
From ya.ru downloaded 15 bytes, content: "I am form ya.ru"
```

## API Reference
Effil provides:
- `effil.thread`
- `effil.table`
- `effil.channel`
- set of helper functions.

#### Implementation
All effil features implemented in C++ with great help of [sol2](https://github.com/ThePhD/sol2).
It requires C++14 compliance (GCC 4.9, Visual Studio 2015 and clang 3.?).

### effil.thread
#### Overview
`effil.thread` is a way to create thread. Threads can be stopped, paused, resumed and canceled.
All operation with threads can be synchronous (with or without timeout) or asynchronous. 
Each thread runs with its own lua state.
**Do not run function with upvalues in `effil.thread`**  
Use `effil.table` and `effil.channel` to transmit data over threads.

#### Example
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

#### API
`runner = effil.threas(f)` - creates thread runner. Runner spawn new thread for each invocation.  
#### Thread runner
- `runner.path` - `package.path` value for new state. Default value inherits `package.path` form parent state.
- `runner.path` - `package.cpath` value for new state. Default value inherits `package.cpath` form parent state.
- `runner.step` - number of lua instructions lua between cancelation points. Default value is 200. Spawn unstopable threads when value is equal to 0.    
- `thr = runner(arg1, arg2, arg3)` - run captured function with this args in separate thread and returns handle.

#### Thread handle
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
- `s` - seconds (default);
- `m` - minutes;
- `h` - hours.

#### Thread helpers
`effil.thread_id()` - unique string thread id. 
`effil.yield()` - explicit cancellation point.
`effil.sleep(time, metric)` - suspend current thread. `metric` is optional and default is seconds.

### effil.table
#### Overview
`effil.table` is a way to exchange data between effil threads.
It behaves almost like standard lua tables.
All operations with shared table are thread safe.
**Shared table stores** primitive types (number, boolean, string),
function, table, light userdata and effil based userdata.
**Shared table doesn't store** lua threads (coroutines) or arbitrary userdata.

#### Example
```lua
local effil = require "effil"
local pets = effil.table()
pets.sparky = effil.table { says = "wof" }
assert(pets.sparky.says == "wof")
```

#### API
- `effil.table()` - create new empty shared table.
- `effil.table(tbl)` - create new shared table and fill it with values from `tbl`.
- `effil.size(tbl)` - get number of entries in table.
- `effil.setmetatable(tbl, mtbl)` - set metatable. `mtbl` can be regular or shared table. 
- `effil.getmetatable(tbl)` - returns current metatable. Returned table always has type `effil.table`. Default metatable is `nil`.
- `effil.rawset(tbl, key, value)` - set table entry without invoking metamethod `__newindex`.
- `effil.rawget(tbl, key)` - get table value without invoking metamethod `__index`.

#### Shared tables with regular tables
If you want to store regular table in shared table, effil will implicitly dump origin table into new shared table.
**Shared tables always stores subtables as shared tables.**

#### Shared tables with functions
If you want to store function in shared table, effil will implicitly dump this function and saves it in internal representation as string.
Thus, all upvalues will be lost.
**Do not store function with upvalues in shared tables**.

#### Global shred table
`effil.G` is a global predefined shared table. 
This table always present in any thread.  

#### Type identification 
Use `effil.type` to deffer effil.table for other userdata.
```lua
assert(effil.type(effil.table()) == "effil.table")
```

### effil.channel
#### Overview
`effil.channel` is a way to sequentially exchange data between effil threads.
It allows push values from one thread and pop them from another.
All operations with channels are thread safe.
**Channel passes** primitive types (number, boolean, string),
function, table, light userdata and effil based userdata.
**Channel doesn't pass** lua threads (coroutines) or arbitrary userdata.

#### Example
```lua
local effil = require "effil"

local chan = effil.channel()
chan:push(1, "Wow")
chan:push(2, "Bark")

local n, s = chan:pop()
assert(1 == n)
assert("Wow" == s)
assert(chan:size() == 1)
```

#### API
- `chan = effil.channel(capacity)` - channel capacity. If `capacity` equals to `0` size of channel is unlimited. Default capacity is `0`.
- `chan:push()` - push value. Returns `true` if value fits channel capacity, `false` otherwise. Supports multiple values.
- `chan:pop()` - pop value. If value is not present, wait for the value.
- `chan:pop(time, metric)` - pop value with timeout. If time equals `0` then pop asynchronously.
- `chan:size()` - get actual size of channel.

#### Metrics
- `ms` - milliseconds;
- `s` - seconds;
- `m` - minutes;
- `h` - hours.

### effil.type
Threads, channels and tables are userdata.
Thus, `type()` will return `userdata` for any type.
If you want to detect type more precisely use `effil.type`.
It behaves like regular `type()`, but it can detect effil specific userdata.
There is a list of extra types:
- `effil.type(effil.thread()) == "effil.thread"`
- `effil.type(effil.table()) == "effil.table"`
- `effil.type(effil.channel() == "effil.channel"`

### effil.gc
#### Overview
Effil provides custom garbage collector for `effil.table` and `effil.table`.
It allows safe manage cyclic references for tables and channels in multiple threads.
However it may cause extra memory usage. 
`effil.gc` provides a set of method configure effil garbage collector.
But, usually you don't need to configure it.

#### Garbage collection trigger
Garbage collection may occur with new effil object creation (table or channel).
Frequency of triggering configured by GC step.
For example, if Gc step is 200, then each 200'th object creation trigger GC.    

#### API
- `effil.gc.collect()` - force garbage collection, however it doesn't guarantee deletion of all effil objects.
- `effil.gc.count()` - show number of allocated shared tables and channels. Minimum value is 1, `effil.G` is always present. 
- `effil.gc.step()` - get GC step. Default is `200`.
- `effi.gc.step(value)` - set GC step and get previous value. 
- `effil.gc.pause()` - pause GC.
- `effil.gc.resume()` - resume GC.
- `effil.gc.enabled()` - get GC state.

#### How to cleanup all dereferenced objects 
Each thread represented as separate state with own garbage collector.
Thus, objects will be deleted eventually.
Effil objects itself also managed by GC and uses `__gc` userdata metamethod as deserializer hook.
To force objects deletion:
1. invoke `collectgarbage()` in all threads.
2. invoke `effil.gc.collect()` in any thread.
