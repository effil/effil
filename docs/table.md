# effil.table
=============
### Overview
`effil.table` is a way to exchange data between effil threads.
It behaves almost like standard lua tables.
All operations with shared table are thread safe.
**Shared table stores** primitive types (number, boolean, string),
function, table, light userdata and effil based userdata.
**Shared table doesn't store** lua threads (coroutines) or arbitrary userdata.

### Example
```lua
local effil = require "effil"
local pets = effil.table()
pets.sparky = effil.table { says = "wof" }
assert(pets.sparky.says == "wof")
```

### API
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

### Type identification 
Use `effil.type` to deffer effil.table for other userdata.
```lua
assert(effil.type(effil.table()) == "effil.table")
```

