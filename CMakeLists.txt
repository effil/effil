cmake_minimum_required(VERSION 2.8)
project(woofer)

find_package(Lua REQUIRED)

if( "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}" LESS "5.2")
    message(FATAL_ERROR "Wrong Lua version ${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}, use 5.2 or higher")
endif()

include_directories(src src/sol/single/sol ${LUA_INCLUDE_DIR})

FILE(GLOB SOURCES src/*.cpp src/*.h)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

add_library(woofer SHARED ${SOURCES})
target_link_libraries(woofer -lpthread ${LUA_LIBRARY})

set(GENERAL "-std=c++14 -pthread")
set(ENABLE_WARNINGS "-Wall -Wextra -pedantic")
set(BUILD_FLAVOR "-O3 -UNDEBUG")
set_target_properties(woofer PROPERTIES COMPILE_FLAGS "${ENABLE_WARNINGS} ${GENERAL} ${BUILD_FLAVOR}")

#----------
# INSTALL -
#----------

install(FILES ${LUA_SOURCES} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})