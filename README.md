# Effil
[![Build Status](https://travis-ci.org/loud-hound/effil.svg?branch=master)](https://travis-ci.org/loud-hound/effil)
[![Build Status Windows](https://ci.appveyor.com/api/projects/status/us6uh4e5q597jj54?svg=true)](https://ci.appveyor.com/project/loud-hound/effil/branch/master)
[![Documentation Status](https://readthedocs.org/projects/effil/badge/?version=latest)](http://effil.readthedocs.io/en/latest/?badge=latest)

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
