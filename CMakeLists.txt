cmake_minimum_required(VERSION 2.8)
project(effil)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

if (NOT (LUA_INCLUDE_DIR OR BUILD_ROCK))
    find_package(Lua REQUIRED)
endif()

# avoid the extra "Debug", "Release" directories
# http://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
endforeach()

# Supress warning CMP0042
if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

# libeffil not directly linked with liblua
if (APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

include_directories(src/cpp libs/sol/single/sol ${LUA_INCLUDE_DIR})

FILE(GLOB SOURCES src/cpp/*.cpp src/cpp/*.h)
FILE(GLOB LUA_SOURCES src/lua/*.lua)

add_library(effil SHARED ${SOURCES})
target_link_libraries(effil ${LUA_LIBRARY})
if (WIN32)
    set_target_properties(effil PROPERTIES
        PREFIX ""
        SUFFIX .dll)
else()
    target_link_libraries(effil -lpthread -ldl)
    set_target_properties(effil PROPERTIES
        PREFIX ""
        SUFFIX .so)
endif()

set(GENERAL "-DSOL_EXCEPTIONS_SAFE_PROPAGATION")
set(CMAKE_CXX_FLAGS "${EXTRA_FLAGS} ${CMAKE_CXX_FLAGS} ${GENERAL}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0")
endif()

#----------
# INSTALL -
#----------
install(TARGETS effil
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
