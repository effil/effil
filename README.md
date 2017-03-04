# Effil
Lua library for real multithreading. Written in C++ with great help of [sol2](https://github.com/ThePhD/sol2).

[![Build Status](https://travis-ci.org/loud-hound/effil.svg?branch=master)](https://travis-ci.org/loud-hound/effil)
[![Documentation Status](https://readthedocs.org/projects/effil/badge/?version=latest)](http://effil.readthedocs.io/en/latest/?badge=latest)

## How to install
### Build from src on Linux and Mac
1. `git clone git@github.com:loud-hound/effil.git effil && cd effil`
2. `mkdir build && cd build && make -j4 `
3. Add libeffil.so or libeffil.dylib to your lua `package.path`.

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
local effil = require("libeffil")
function bark(name)
    print(name .. ": bark")
end

-- associate bark with thread
-- invoke bark in separate thread with spark argument
-- wait while Sparky barks
effil.thread(bark)("Sparky"):wait()
```
Output: `Sparky: bark`
### Sharing data
```lua

local effil = require("libeffil")

function download_heavy_file(url, files)
    -- i am to lazy to write real downloading here
    files[url] = "content of " .. url
end

-- shared table for data exchanging
local files = effil.table {}
local urls = {"luarocks.org", "ya.ru", "github.com"}
local downloads = {}
-- capture function for further threads
local downloader = effil.thread(download_heavy_file)

for i, url in pairs(urls) do
    -- run downloads in separate threads
    -- each invocation creates separate thread
    downloads[url] = downloader(url, files)
end

for i, url in pairs(urls) do
    -- here we go
    downloads[url]:wait()
    print("Downloaded: "  .. files[url])
end

```
Output:
```
Downloaded:File contentluarocks.org
Downloaded:File contentya.ru
Downloaded:File contentgithub.com
```

## Reference
There is no