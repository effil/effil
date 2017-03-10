local function detect_native_lib_ext()
    local home = os.getenv("HOME")
    if not home then return "dll" end
    if string.find(home, "/Users/") then return "dylib" end
    if string.find(home, "/home/") then return "so" end
    -- TODO: unable to detect os
    -- how to reportabout error
    return "so"
end

package.cpath = package.cpath .. ";./?." .. detect_native_lib_ext()

local api = require 'libeffil'

return api