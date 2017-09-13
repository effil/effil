package = "effil"
version = "1.0-0"

source = {
    url = "gitrec+http://github.com/effil/effil",
    tag = "v1.0-0"
}

description = {
    summary  = "Multithreading library for Lua.",
    detailed = [[
       Effil is a library provides multithreading support for Lua.
       *luarocks install luarocks-fetch-gitrec*
    ]],
    homepage = "http://github.com/loud-hound/effil",
    license  = "MIT"
}

dependencies = {
    "lua >= 5.1"
}

local function get_unix_build()
    local install_dir = "rockspeck-content"
    return {
        type = "cmake",
        variables = {
            CMAKE_BUILD_TYPE     = "Release",
            CMAKE_PREFIX_PATH    = "$(LUA_BINDIR)/..",
            CMAKE_INSTALL_PREFIX = install_dir,
            CMAKE_LIBRARY_PATH   = "$(LUA_LIBDIR)",
            LUA_INCLUDE_DIR      = "$(LUA_INCDIR)",
            BUILD_ROCK           = "yes"
        },
      install = {
          lua = { install_dir .. "/effil.lua" },
          lib = { install_dir .. "/libeffil.so" }
      }
    }
end

build = {
    platforms = {
        linux = get_unix_build(),
        macosx = get_unix_build()
    }
}
