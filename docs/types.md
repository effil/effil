# effil.type
============
Threads, channels and tables are userdata.
Thus, `type()` will return `userdata` for any type.
If you want to detect type more precisely use `effil.type`.
It behaves like regular `type()`, but it can detect effil specific userdata.
There is a list of extra types:
- `effil.type(effil.thread()) == "effil.thread"`
- `effil.type(effil.table()) == "effil.table"`
- `effil.type(effil.channel() == "effil.channel"`
