cmake_minimum_required(VERSION 2.8)
project(effil)

if (NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARY)
    find_package(Lua REQUIRED)
endif()

include_directories(src/cpp libs/sol/single/sol ${LUA_INCLUDE_DIR})

FILE(GLOB SOURCES src/cpp/*.cpp src/cpp/*.h)
FILE(GLOB LUA_SOURCES src/lua/*.lua)

if(APPLE)
    # Supress warning CMP0042
    set(CMAKE_MACOSX_RPATH 1)
    set(LIBRARY_EXT dylib)
else()
    set(LIBRARY_EXT so)
endif()

add_library(effil SHARED ${SOURCES})
target_link_libraries(effil -lpthread ${LUA_LIBRARY} -ldl)

set(GENERAL "-std=c++14 -DSOL_EXCEPTIONS_SAFE_PROPAGATION")
set(ENABLE_WARNINGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GENERAL} ${ENABLE_WARNINGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror -O0 -g -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0 -DNDEBUG")

#----------
# TESTS ---
#----------
FILE(GLOB TEST_SOURCES tests/cpp/*.cpp tests/cpp/*.h)
set(GTEST_DIR libs/gtest/googletest)
set(LUA_TESTS tests/lua/tests.lua)

include_directories(${GTEST_DIR}/include ${GTEST_DIR})
add_executable(tests ${TEST_SOURCES} ${GTEST_DIR}/src/gtest-all.cc)
target_link_libraries(tests effil ${LUA_LIBRARY})

#----------
# FORMAT  -
#----------
add_custom_target( format
        COMMAND clang-format -i ${TEST_SOURCES} ${SOURCES}
)

#----------
# INSTALL -
#----------
install(FILES ${LUA_TESTS}
        DESTINATION ${CMAKE_BINARY_DIR}
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
