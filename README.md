# Effil
[![Generic badge](https://img.shields.io/badge/Lua-5.1-44cc11.svg)](https://shields.io/)
[![Generic badge](https://img.shields.io/badge/Lua-5.2-44cc11.svg)](https://shields.io/)
[![Generic badge](https://img.shields.io/badge/Lua-5.3-44cc11.svg)](https://shields.io/)
[![Generic badge](https://img.shields.io/badge/LuaJIT-2.0-44cc11.svg)](https://shields.io/)

[![LuaRocks](https://img.shields.io/luarocks/v/mihacooper/effil.svg)](https://luarocks.org/modules/mihacooper/effil)
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)]()
[![Join the chat at https://gitter.im/effil-chat/Lobby](https://badges.gitter.im/effil-chat/Lobby.svg)](https://gitter.im/effil-chat/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

| Linux + MacOS | Windows |
| ------------- | ------- |
| [![Build Status](https://travis-ci.org/effil/effil.svg?branch=master)](https://travis-ci.org/effil/effil) | [![Build status](https://ci.appveyor.com/api/projects/status/ji0l9lbs17djgm8w/branch/master?svg=true)](https://ci.appveyor.com/project/mihacooper/effil/branch/master) |

Effil is a multithreading library for Lua.
It allows to spawn native threads and safe data exchange.
Effil has been designed to provide clear and simple API for lua developers.

Effil supports lua 5.1, 5.2, 5.3 and LuaJIT.
Requires C++14 compiler compliance. Tested with GCC 4.9+, clang 3.8 and Visual Studio 2015.

# Table Of Contents
* [How to install](#how-to-install)
* [Quick guide](#quick-guide)
* [Important notes](#important-notes)
* [Blocking and nonblocking operations](#blocking-and-nonblocking-operations)
* [Function's upvalues](#functions-upvalues)
* [API Reference](#api-reference)
   * [Thread](#thread)
      * [effil.thread()](#runner--effilthreadfunc)
    * [Thread runner](#thread-runner)
      * [runner()](#thread--runner)
      * [runner.path](#runnerpath)
      * [runner.cpath](#runnercpath)
      * [runner.step](#runnerstep)
    * [Thread handle](#thread-handle)
      * [thread:status()](#status-err-stacktrace--threadstatus)
      * [thread:get()](#--threadgettime-metric)
      * [thread:wait()](#threadwaittime-metric)
      * [thread:cancel()](#threadcanceltime-metric)
      * [thread:pause()](#threadpausetime-metric)
      * [thread:resume()](#threadresume)
    * [Thread helpers](#thread-helpers)
      * [effil.thread_id()](#id--effilthread_id)
      * [effil.yield()](#effilyield)
      * [effil.sleep()](#effilsleeptime-metric)
      * [effil.hardware_threads()](#effilhardware_threads)
    * [Table](#table)
      * [effil.table()](#table--effiltabletbl)
      * [__newindex: table[key] = value](#tablekey--value)
      * [__index: value = table[key]](#value--tablekey)
      * [effil.setmetatable()](#tbl--effilsetmetatabletbl-mtbl)
      * [effil.getmetatable()](#mtbl--effilgetmetatabletbl)
      * [effil.rawset()](#tbl--effilrawsettbl-key-value)
      * [effil.rawget()](#value--effilrawgettbl-key)
      * [effil.G](#effilg)
      * [effil.dump()](#result--effildumpobj)
    * [Channel](#channel)
      * [effil.channel()](#channel--effilchannelcapacity)
      * [channel:push()](#pushed--channelpush)
      * [channel:pop()](#--channelpoptime-metric)
      * [channel:size()](#size--channelsize)
    * [Garbage collector](#garbage-collector)
      * [effil.gc.collect()](#effilgccollect)
      * [effil.gc.count()](#count--effilgccount)
      * [effil.gc.step()](#old_value--effilgcstepnew_value)
      * [effil.gc.pause()](#effilgcpause)
      * [effil.gc.resume()](#effilgcresume)
      * [effil.gc.enabled()](#enabled--effilgcenabled)
    * [Other methods](#othermethods)
      * [effil.size()](#size--effilsizeobj)
      * [effil.type()](#effiltype)

# How to install
### Build from src on Linux and Mac
1. `git clone --recursive git@github.com:effil/effil.git effil`
2. `cd effil && mkdir build && cd build`
3. `cmake .. && make install`
4. Copy effil.so to your project.

### From lua rocks
`luarocks install effil`

# Quick guide
As you may now there are not much script languages with **real** multithreading support (Lua/Python/Ruby and etc has global interpreter lock aka GIL). Effil solves this problem by running independent Lua VM instances in separate native threads and provides robust communicating primitives for creating threads and data sharing.

Effil library provides three major abstractions:
1. `effil.thread` - provides API for threads management.
2. `effil.table` - provides API for tables management. Tables can be shared between threads.
3. `effil.channel` - provides First-In-First-Out container for sequential data exchange.

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

-- channel allow to push data in one thread and pop in other
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
effil = require("effil")

-- effil.table transfers data between threads
-- and behaves like regualr lua table
local storage = effil.table { string_field = "first value" }
storage.numeric_field = 100500
storage.function_field = function(a, b) return a + b end
storage.table_field = { fist = 1, second = 2 }

function check_shared_table(storage)
   print(storage.string_field)
   print(storage.numeric_field)
   print(storage.table_field.first)
   print(storage.table_field.second)
   return storage.function_field(1, 2)
end

local thr = effil.thread(check_shared_table)(storage)
local ret = thr:get()
print("Thread result: " .. ret)

```
**Output:**
```
first value
100500
1
2
Thread result: 3
```
   </p>
</details>

# Important notes
Effil allows to transmit data between threads (Lua interpreter states) using `effil.channel`, `effil.table` or directly as parameters of `effil.thread`.
 - Primitive types are transmitted 'as is' by copy: `nil`, `boolean`, `number`, `string`
 - Functions are dumped using [`lua_dump`](#https://www.lua.org/manual/5.3/manual.html#lua_dump). Upvalues are captured according to [the rules](#functions-upvalues).
 - **Userdata and Lua threads (coroutines)** are not supported.
 - Tables are serialized to `effil.table` recursively. So, any Lua table becomes `effil.table`. Table serialization may take a lot of time for big table. Thus, it's better to put data directly to `effil.table` avoiding a table serialization. Let's consider 2 examples:
```Lua
-- Example #1
t = {}
for i = 1, 100 do
   t[i] = i
end
shared_table = effil.table(t)

-- Example #2
t = effil.table()
for i = 1, 100 do
   t[i] = i
end
```
In the example #1 we create regular table, fill it and convert it to `effil.table`. In this case Effil needs to go through all table fields one more time. Another way is example #2 where we firstly created `effil.table` and after that we put data directly to `effil.table`. The 2nd way pretty much faster try to follow this principle.

## Blocking and nonblocking operations:
All operations which use time metrics can be blocking or non blocking and use following API:
`(time, metric)` where `metric` is time interval like `'s'` (seconds) and `time` is a number of intervals.

Example:
- `thread:get()` - infinitely wait for thread completion.
- `thread:get(0)` - non blocking get, just check is thread finished and return
- `thread:get(50, "ms")` - blocking wait for 50 milliseconds.

List of available time intervals:
- `ms` - milliseconds;
- `s` - seconds (default);
- `m` - minutes;
- `h` - hours.

## Function's upvalues
Working with functions Effil serializes and deserializes them using [`lua_dump`](#https://www.lua.org/manual/5.3/manual.html#lua_dump) and [`lua_load`](#https://www.lua.org/manual/5.3/manual.html#lua_load) methods. All function's upvalues are stored following the same [rules](#important-notes) as usual. If function has **upvalue of unsupported type** this function cannot be transmitted to Effil. You will get error in that case.

Working with function Effil can store function environment (`_ENV`) as well. Considering environment as a regular table Effil will store it in the same way as any other table. But it does not make sence to store global `_G`, so there are some specific:
 * *Lua = 5.1*: function environment is not stored at all (due to limitations of lua_setfenv we cannot use userdata)
 * *Lua > 5.1*: Effil serialize and store function environment only if it's not equal to global environment (`_ENV ~= _G`).

# API Reference

## Thread
`effil.thread` is the way to create a thread. Threads can be stopped, paused, resumed and canceled.
All operation with threads can be synchronous (with optional timeout) or asynchronous.
Each thread runs with its own lua state.

Use `effil.table` and `effil.channel` to transmit data over threads.
See example of thread usage [here](#examples).

### `runner = effil.thread(func)`
Creates thread runner. Runner spawns new thread for each invocation. 

**input**: *func* - Lua function

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
Thread handle provides API for interaction with thread.

### `status, err, stacktrace = thread:status()`
Returns thread status.

**output**:
- `status` - string values describes status of thread. Possible values are: `"running", "paused", "canceled", "completed" and "failed"`.
- `err` - error message, if any. This value is specified only if thread status == `"failed"`.
- `stacktrace` - stacktrace of failed thread. This value is specified only if thread status == `"failed"`.

### `... = thread:get(time, metric)`
Waits for thread completion and returns function result or nothing in case of error.

**input**: Operation timeout in terms of [time metrics](#blocking-and-nonblocking-operations)

**output**: Results of captured function invocation or nothing in case of error.

### `thread:wait(time, metric)`
Waits for thread completion and returns thread status.

**input**: Operation timeout in terms of [time metrics](#blocking-and-nonblocking-operations)

**output**: Returns status of thread. The output is the same as [`thread:status()`](#status-err--threadstatus)

### `thread:cancel(time, metric)`
Interrupts thread execution. Once this function was invoked 'cancellation' flag  is set and thread can be stopped sometime in the future (even after this function call done). To be sure that thread is stopped invoke this function with infinite timeout. Cancellation of finished thread will do nothing and return `true`.

**input**: Operation timeout in terms of [time metrics](#blocking-and-nonblocking-operations)

**output**: Returns `true` if thread was stopped or `false`.

### `thread:pause(time, metric)`
Pauses thread. Once this function was invoked 'pause' flag  is set and thread can be paused sometime in the future (even after this function call done). To be sure that thread is paused invoke this function with infinite timeout.

**input**: Operation timeout in terms of [time metrics](#blocking-and-nonblocking-operations)

**output**: Returns `true` if thread was paused or `false`. If the thread is completed function will return `false`

### `thread:resume()`
Resumes paused thread. Function resumes thread immediately if it was paused. This function does nothing for completed thread. Function has no input and output parameters.

## Thread helpers
### `id = effil.thread_id()`
Gives unique identifier.

**output**:  returns unique string `id` for *current* thread.

### `effil.yield()`
Explicit cancellation point. Function checks *cancellation* or *pausing* flags of current thread and if it's required it performs corresponding actions (cancel or pause thread).

### `effil.sleep(time, metric)`
Suspend current thread.

**input**: [time metrics](#blocking-and-nonblocking-operations) arguments.

### `effil.hardware_threads()`
Returns the number of concurrent threads supported by implementation.
Basically forwards value from [std::thread::hardware_concurrency](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency).
**output**: number of concurrent hardware threads.

## Table
`effil.table` is a way to exchange data between effil threads. It behaves almost like standard lua tables.
All operations with shared table are thread safe. **Shared table stores** primitive types (number, boolean, string), function, table, light userdata and effil based userdata. **Shared table doesn't store** lua threads (coroutines) or arbitrary userdata. See examples of shared table usage [here](#examples)

### Notes: shared tables usage

Use **Shared tables with regular tables**. If you want to store regular table in shared table, effil will implicitly dump origin table into new shared table. **Shared tables always stores subtables as shared tables.**

Use **Shared tables with functions**. If you store function in shared table, effil implicitly dumps this function and saves it as string (and it's upvalues). All function's upvalues will be captured according to [following rules ](#functions-upvalues).

### `table = effil.table(tbl)`
Creates new **empty** shared table.

**input**: `tbl` - is *optional* parameter, it can be only regular Lua table which entries will be **copied** to shared table.

**output**: new instance of empty shared table. It can be empty or not, depending on `tbl` content.

### `table[key] = value` 
Set a new key of table with specified value.

**input**:
- `key` - any value of supported type. See the list of [supported types](#important-notes)
- `value` - any value of supported type. See the list of [supported types](#important-notes)

### `value = table[key]` 
Get a value from table with specified key.

**input**: `key` - any value of supported type. See the list of [supported types](#important-notes)

**output**: `value` - any value of supported type. See the list of [supported types](#important-notes)

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
- `key` - key of table to override. The key can be of any [supported type](#important-notes).
- `value` - value to set. The value can be of any [supported type](#important-notes).

**output**: returns the same shared table `tbl`

### `value = effil.rawget(tbl, key)`
Gets table value without invoking metamethod `__index`. Similar to standard [rawget](https://www.lua.org/manual/5.3/manual.html#pdf-rawget)

**input**:
- `tbl` is shared table. 
- `key` - key of table to receive a specific value. The key can be of any [supported type](#important-notes).

**output**: returns required `value` stored under a specified `key`

### `effil.G`
Is a global predefined shared table. This table always present in any thread (any Lua state).
```lua
effil = require "effil"

function job()
   effil = require "effil"
   effil.G.key = "value"
end

effil.thread(job)():wait()
print(effil.G.key) -- will print "value"
```

### `result = effil.dump(obj)`
Truns `effil.table` into regular Lua table.
```lua
tbl = effil.table({})
effil.type(tbl) -- 'effil.table'
effil.type(effil.dump(tbl))  -- 'table'
```

## Channel
`effil.channel` is a way to sequentially exchange data between effil threads. It allows to push message from one thread and pop  it from another. Channel's **message** is a set of values of [supported types](#important-notes). All operations with channels are thread safe. See examples of channel usage [here](#examples)

### `channel = effil.channel(capacity)`
Creates a new channel.

**input**: optional *capacity* of channel. If `capacity` equals to `0` or to `nil` size of channel is unlimited. Default capacity is `0`.

**output**: returns a new instance of channel.

### `pushed = channel:push(...)`
Pushes message to channel.

**input**: any number of values of [supported types](#important-notes). Multiple values are considered as a single channel message so one *push* to channel decreases capacity by one.

**output**: `pushed` is equal to `true` if value(-s) fits channel capacity, `false` otherwise.

### `... = channel:pop(time, metric)`
Pop message from channel. Removes value(-s) from channel and returns them. If the channel is empty wait for any value appearance.

**input**: waiting timeout in terms of [time metrics](#blocking-and-nonblocking-operations) (used only if channel is empty).

**output**: variable amount of values which were pushed by a single [channel:push()](#pushed--channelpush) call.

### `size = channel:size()`
Get actual amount of messages in channel.

**output**: amount of messages in channel.

## Garbage collector
Effil provides custom garbage collector for `effil.table` and `effil.channel` (and functions with captured upvalues). It allows safe manage cyclic references for tables and channels in multiple threads. However it may cause extra memory usage. `effil.gc` provides a set of method configure effil garbage collector. But, usually you don't need to configure it.

### Garbage collection trigger
Garbage collector perform it's work when effil creates new shared object (table, channel and functions with captured upvalues).
Each iteration GC checks amount of objects. If amount of allocated objects becomes higher then specific threshold value GC starts garbage collecting. Threshold value is calculated as `previous_count * step`, where `previous_count` - amount of objects on previous iteration (**100** by default) and `step` is a numerical coefficient specified by user (**2.0** by default).

For example: if GC `step` is `2.0` and amount of allocated objects is `120` (left after previous GC iteration) then GC will start to collect garbage when amount of allocated objects will be equal to`240`.

### How to cleanup all dereferenced objects 
Each thread represented as separate Lua state with own garbage collector.
Thus, objects will be deleted eventually.
Effil objects itself also managed by GC and uses `__gc` userdata metamethod as deserializer hook.
To force objects deletion:
1. invoke standard `collectgarbage()` in all threads.
2. invoke `effil.gc.collect()` in any thread.

### `effil.gc.collect()`
Force garbage collection, however it doesn't guarantee deletion of all effil objects.

### `count = effil.gc.count()`
Show number of allocated shared tables and channels.

**output**: returns current number of allocated objects. Minimum value is 1, `effil.G` is always present. 

### `old_value = effil.gc.step(new_value)`
Get/set GC memory step multiplier. Default is `2.0`. GC triggers collecting when amount of allocated objects growth in `step` times.

**input**: `new_value` is optional value of step to set. If it's `nil` then function will just return a current value.

**output**: `old_value` is current (if `new_value == nil`) or previous (if `new_value ~= nil`) value of step.

### `effil.gc.pause()`
Pause GC. Garbage collecting will not be performed automatically. Function does not have any *input* or *output*

### `effil.gc.resume()`
Resume GC. Enable automatic garbage collecting.

### `enabled = effil.gc.enabled()`
Get GC state.

**output**: return `true` if automatic garbage collecting is enabled or `false` otherwise. By default returns `true`.

## Other methods

### `size = effil.size(obj)`
Returns number of entries in Effil object.

**input**: `obj` is [shared table](#table) or [channel](#channel).

**output**: number of entries in [shared table](#table) or number of messages in [channel](#channel)

### `type = effil.type(obj)`
Threads, channels and tables are userdata. Thus, `type()` will return `userdata` for any type. If you want to detect type more precisely use `effil.type`. It behaves like regular `type()`, but it can detect effil specific userdata.

**input**: `obj` is object of any type.

**output**: string name of type. If `obj` is Effil object then function returns string like `effil.table` in other cases it returns result of [lua_typename](https://www.lua.org/manual/5.3/manual.html#lua_typename) function.

```Lua
effil.type(effil.thread()) == "effil.thread"
effil.type(effil.table()) == "effil.table"
effil.type(effil.channel()) == "effil.channel"
effil.type({}) == "table"
effil.type(1) == "number"
```
