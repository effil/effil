# effil.channel
===============

### Overview
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
