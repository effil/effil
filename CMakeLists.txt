cmake_minimum_required(VERSION 2.8)
project(effil)

if (NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARY)
    find_package(Lua REQUIRED)
    if( "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}" LESS "5.2")
        message(FATAL_ERROR "Wrong Lua version ${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}, use 5.2 or higher")
    endif()
endif()

include_directories(src src/sol/single/sol ${LUA_INCLUDE_DIR})

FILE(GLOB SOURCES src/*.cpp src/*.h)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

if(APPLE)
    # Supress warning CMP0042
    set(CMAKE_MACOSX_RPATH 1)
endif()

add_library(effil SHARED ${SOURCES})
target_link_libraries(effil -lpthread ${LUA_LIBRARY})

set(GENERAL "-std=c++14 -pthread")
set(ENABLE_WARNINGS "-Wall -Wextra -pedantic -Werror")
set(BUILD_FLAVOR "-O3 -UNDEBUG")
set_target_properties(effil PROPERTIES COMPILE_FLAGS "${ENABLE_WARNINGS} ${GENERAL} ${BUILD_FLAVOR}")

#----------
# TESTS ---
#----------

FILE(GLOB TEST_SOURCES tests/*.cpp)
set(GTEST_DIR tests/gtest/googletest)

include_directories(${GTEST_DIR}/include ${GTEST_DIR})
add_executable(tests ${TEST_SOURCES} ${GTEST_DIR}/src/gtest-all.cc)
target_link_libraries(tests effil)

set_target_properties(tests PROPERTIES COMPILE_FLAGS "${ENABLE_WARNINGS} ${GENERAL} ${BUILD_FLAVOR}")

#----------
# INSTALL -
#----------

install(FILES ${LUA_SOURCES} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
