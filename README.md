# Effil
[![Build Status](https://travis-ci.org/effil/effil.svg?branch=master)](https://travis-ci.org/loud-hound/effil)

Effil is a lua module for multithreading support.
It allows to spawn native threads and safe data exchange.
Effil has been designed to provide clear and simple API for lua developers including threads, channels and shared tables.

Effil supports lua 5.1, 5.2, 5.3 and LuaJIT.
Requires C++14 compiler compliance. Tested with GCC 4.9/5.3, clang 3.8 and Visual Studio 2015.

Read the [docs](https://github.com/loud-hound/effil/blob/master/README.md) for more information

# Table Of Contents
* [How to install](#how-to-install)
* [Quick guide](#quick-guide)
* [API Reference](#api-reference)
    * [effil.thread](#effilthread)
    * [effil.table](#effiltable)
    * [effil.channel](#effilchannel)
    * [effil.thread_id](#effilthread_id)
    * [effil.yield](#effilyield)
    * [effil.sleep](#effilsleeptime-metric)

# How to install
### Build from src on Linux and Mac
1. `git clone git@github.com:loud-hound/effil.git effil && cd effil`
2. `mkdir build && cd build && make -j4 install`
3. Copy effil.lua and libeffil.so/libeffil.dylib to your project.

### From lua rocks
`luarocks install effil`

# Quick guide
As you may now there are not much script
languages with **real** multithreading support
(Lua/Python/Ruby and etc has global interpreter lock aka GIL).
Effil solves this problem by running independent Lua VM
in separate native thread and provides robust communicating primitives
for creating threads (VM instances) and data sharing.

Effil library provides three major functions:
1. `effil.thread` - function which creates threads.
2. `effil.table` - table that persist in all threads and behaves just like regular lua table.
3. `effil.channel` - channel to send and receive data between threads.

And bunch of utilities to handle threads and tables as well.

### Examples
<details>
   <summary><b>Spawn the thread</b></summary>
   <p>

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
   </p>
</details>

<details>
   <summary><b>Sharing data with effil.channel</b></summary>
   <p>

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
   </p>
</details>

<details>
   <summary><b>Sharing data with effil.table</b></summary>
   <p>

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
   </p>
</details>

# API Reference

## effil.thread
`effil.thread` is a way to create thread. Threads can be stopped, paused, resumed and canceled.
All operation with threads can be synchronous (with timeout or infinite) or asynchronous.
Each thread runs with its own lua state.
**Do not run function with upvalues in** `effil.thread`. Use `effil.table` and `effil.channel` to transmit data over threads.
See example of thread usage [here](#thread-usage-example).

### `runner = effil.thread(func)`
Creates thread runner. Runner spawns new thread for each invocation. 

**input**: *func* - any Lua function without upvalues

**output**: *runner* - [thread runner](#thread-runner) object to configure and run a new thread

## Thread runner
Allows to configure and run a new thread.
### `thread = runner(...)`
Run captured function with specified arguments in separate thread and returns [thread handle](#thread-handle).

**input**: Any number of arguments required by captured function.

**output**: [Thread handle](#thread-handle) object.

### `runner.path`
Is a Lua `package.path` value for new state. Default value inherits `package.path` form parent state.
### `runner.cpath`
Is a Lua `package.cpath` value for new state. Default value inherits `package.cpath` form parent state.
### `runner.step`
Number of lua instructions lua between cancelation points (where thread can be stopped or paused). Default value is 200. If this values is 0 then thread uses only [explicit cancelation points](#effilyield).

## Thread handle
Thread handle provides API for interation with child thread.

### `thread:status()`
Returns thread status.

**output**: String values describes status of thread. Possible values are: `"running", "paused", "canceled", "completed" and "failed"`

### `thread:get(time, metric)`
Waits for thread completion and returns function result or nothing in case of error.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Results of captured function invocation or nothing in case of error.

### `thread:wait(time, metric)`
Waits for thread completion and returns thread status.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Returns status of thread. See the list of possible [statuses](#threadstatus)

### `thread:cancel(time, metric)`
Interrupts thread execution. Once this function was invoked a 'cancellation' flag  is set and thread can be stopped sometime in the future (even after this function call done). To be sure that thread is stopped invoke this function with infinite timeout. Cancellation of finished thread will do nothing and return `true`.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Returns `true` if thread was stopped or `false`.

### `thread:pause(time, metric)`
Pauses thread. Once this function was invoked a 'pause' flag  is set and thread can be paused sometime in the future (even after this function call done). To be sure that thread is paused invoke this function with infinite timeout.

**input**: Operation timeout in terms of [time metrics](#time-metrics)

**output**: Returns `true` if thread was paused or `false`.

### `thread:resume()`
Resumes paused thread. Function resumes thread immediately if it was paused. Function has no input and output parameters.

## Time metrics:
All operations which use time metrics can be bloking or non blocking and use following API:
`(time, metric)` where `metric` is time interval like and `time` is a number of intervals.

Example:
- `thread:get()` - infinitely wait for thread completion.
- `thread:get(0)` - non blocking get, just check is thread finished and return
- `thread:get(50, "ms")` - blocking wait for 50 milliseconds.

List of available time intervals:
- `ms` - milliseconds;
- `s` - seconds (default);
- `m` - minutes;
- `h` - hours.

## Thread helpers
### `effil.thread_id()`
Returns unique string id for *current* thread.
### `effil.yield()`
Explicit cancellation point. Function checks *cancellation* or *pausing* flags of current thread and if it's required it performs corresponding actions (cancel or pause thread).
### `effil.sleep(time, metric)`
Suspend current thread using [time metrics](#time-metrics).

## Thread usage examples
<details>
   <summary><b>Simple example</b></summary>
   <p>

```lua
effil = require "effil"

function greeting(name)
    return "Hello, " .. name
end

runner = effil.thread(greeting) -- capture function to thread runner
thread = runner("Sparky") -- run thread with specific argument

print(thread:get()) -- wait for thread completion and get the result
```
   </p>
</details>

## effil.table
`effil.table` is a way to exchange data between effil threads. It behaves almost like standard lua tables.
All operations with shared table are thread safe. **Shared table stores** primitive types (number, boolean, string), function, table, light userdata and effil based userdata. **Shared table doesn't store** lua threads (coroutines) or arbitrary userdata. See examples of shared table usage [here](#shared-table-usage-examples)

### `table = effil.table()`
Creates new **empty** shared table.

**output**: new instance of empty shared table

### `table = effil.table(tbl)`
Creates new shared table and fill it with values from table `tbl`.

**input**: `tbl` is regular Lua table which entries will be **copied** to shared table.

**output**: new instance of shared table. It can be empty or not, depending on `tbl` content.

### `size = effil.size(tbl)`
Returns number of entries in shared table.

**input**: `tbl` is [shared table](#effiltable) or [channel](#effilchannel) Lua table which entries will be **copied** to shared table.

**output**: new instance of shared table

### `tbl = effil.setmetatable(tbl, mtbl)`
Sets a new metatable to shared table. Similar to standard [setmetatable](https://www.lua.org/manual/5.3/manual.html#pdf-setmetatable).

**input**:
- `tbl` should be shared table for which you want to set metatable. 
- `mtbl` should be regular table or shared table which will become a metatable. If it's a regular table *effil* will create a new shared table and copy all fields of `mtbl`. Set `mtbl` equal to `nil` to delete metatable from shared table.

**output**: just returns `tbl` with a new *metatable* value similar to standard Lua *setmetatable* method.

### `mtbl = effil.getmetatable(tbl)`
Returns current metatable. Similar to standard [getmetatable](https://www.lua.org/manual/5.3/manual.html#pdf-getmetatable)

**input**: `tbl` should be shared table. 

**output**: returns *metatable* of specified shared table. Returned table always has type `effil.table`. Default metatable is `nil`.

### `tbl = effil.rawset(tbl, key, value)`
Set table entry without invoking metamethod `__newindex`. Similar to standard [rawset](https://www.lua.org/manual/5.3/manual.html#pdf-rawset)

**input**:
- `tbl` is shared table. 
- `key` - key of table to override. The key can be of any supported type.
- `value` - value to set. The value can be of any supported type.

**output**: returns the same shared table `tbl`

### `value = effil.rawget(tbl, key)`
Gets table value without invoking metamethod `__index`. Similar to standard [rawget](https://www.lua.org/manual/5.3/manual.html#pdf-rawget)

**input**:
- `tbl` is shared table. 
- `key` - key of table to receive a specific value. The key can be of any supported type.

**output**: returns required `value` stored under a specified `key`

## Notes: shared tables usage

Use **Shared tables with regular tables**. If you want to store regular table in shared table, effil will implicitly dump origin table into new shared table. **Shared tables always stores subtables as shared tables.**

Use **Shared tables with functions**. If you want to store function in shared table, effil will implicitly dump this function and saves it in internal representation as string. Thus, all upvalues will be lost. **Do not store function with upvalues in shared tables**.

## Shared table usage examples
<details>
   <summary><b>Simple example</b></summary>
   <p>

```lua
local effil = require "effil"
local pets = effil.table()
pets.sparky = effil.table { says = "wof" }
assert(pets.sparky.says == "wof")
```
   </p>
</details>

## `effil.G`
Is a global predefined shared table. This table always present in any thread (any Lua state).
```lua
effil = require "effil"

function job()
   effil = require "effil"
   effil.G.key = "value"
end

effil.G.key == "value"
```

## `effil.type`
Use  to deffer effil.table for other userdata.
```lua
effil.type(effil.channel()) == "effil.channel"
effil.type(effil.table()) == "effil.table"
effil.type(effil.thread()) == "effil.thread"
```

## effil.channel
`effil.channel` is a way to sequentially exchange data between effil threads. It allows push values from one thread and pop them from another. All operations with channels are thread safe. **Channel passes** primitive types (number, boolean, string), function, table, light userdata and effil based userdata. **Channel doesn't pass** lua threads (coroutines) or arbitrary userdata.

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
