--local thr = require('libbevy')
package.cpath = package.cpath .. ";./?.dylib" -- MAC OS support
local api = require('libwoofer')
api.thread.thread_id = nil
api.thread.join = nil
return api