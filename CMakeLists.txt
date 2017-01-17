cmake_minimum_required(VERSION 2.8)
project(bevy)

find_package(Lua REQUIRED)

if( "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}" LESS "5.2")
    message(FATAL_ERROR "Wrong Lua version ${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}, use 5.2 or higher")
endif()

include_directories(src/lib src/sol2/single/sol ${LUA_INCLUDE_DIR})

FILE(GLOB SOURCES src/lib/*.cpp)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

add_library(bevy SHARED ${SOURCES})
target_link_libraries(bevy -lpthread ${LUA_LIBRARY})
set_target_properties(bevy PROPERTIES COMPILE_FLAGS "-Wall -Wextra -pedantic -O3 -g -std=c++14 -pthread")

#----------
# INSTALL -
#----------

FILE(GLOB TESTS tests/*)
FILE(GLOB LUA_SOURCES src/*.lua)
install(FILES ${TESTSz} ${LUA_SOURCES} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})