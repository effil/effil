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
set(ENABLE_WARNINGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS "${EXTRA_FLAGS} ${CMAKE_CXX_FLAGS} ${GENERAL} ${ENABLE_WARNINGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror -O0 -g -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0 -DNDEBUG")

#----------
# TESTS ---
#----------
if (NOT BUILD_ROCK)
    FILE(GLOB TEST_SOURCES tests/cpp/*.cpp tests/cpp/*.h)
    set(GTEST_DIR libs/gtest/googletest)
    set(LUA_TESTS tests/lua/tests.lua)

    include_directories(${GTEST_DIR}/include ${GTEST_DIR})
    add_executable(tests ${TEST_SOURCES} ${GTEST_DIR}/src/gtest-all.cc)
    target_link_libraries(tests effil ${LUA_LIBRARY})
endif()

#----------
# INSTALL -
#----------
install(FILES src/lua/effil.lua
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

install(TARGETS effil
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
