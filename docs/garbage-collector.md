# effil.gc
==========

### Overview
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
