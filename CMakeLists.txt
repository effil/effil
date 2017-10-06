cmake_minimum_required(VERSION 2.8)
project(effil)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

if (NOT (LUA_INCLUDE_DIR AND LUA_LIBRARY OR BUILD_ROCK))
    find_package(Lua REQUIRED)
endif()

# Supress warning CMP0042
if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

# libeffil not directly linked with liblua
if (APPLE AND BUILD_ROCK)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

include_directories(src/cpp libs/sol/single/sol ${LUA_INCLUDE_DIR})

FILE(GLOB SOURCES src/cpp/*.cpp src/cpp/*.h)
FILE(GLOB LUA_SOURCES src/lua/*.lua)

add_library(effil SHARED ${SOURCES})
target_link_libraries(effil -lpthread ${LUA_LIBRARY} -ldl)
set_target_properties(effil PROPERTIES SUFFIX .so)

set(GENERAL "-std=c++14 -DSOL_EXCEPTIONS_SAFE_PROPAGATION")
if(CMAKE_COMPILER_IS_GNUCXX)
    set(ENABLE_WARNINGS "-Wall -Wextra -pedantic")
endif()
set(CMAKE_CXX_FLAGS "${EXTRA_FLAGS} ${CMAKE_CXX_FLAGS} ${GENERAL} ${ENABLE_WARNINGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -UNDEBUG")
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -werror")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0 -DNDEBUG")

#----------
# INSTALL -
#----------
install(FILES src/lua/effil.lua
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

install(TARGETS effil
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
