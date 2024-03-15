cmake_minimum_required(VERSION 3.10.2)

project(redbase)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

message("CMAKE_SYSTEM NAME: ${CMAKE_SYSTEM_NAME}")
message("CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_PLATFORM android)
else()
    message(FATAL_ERROR "This CMake script currently only supports Android!")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(SRC_LIST
    src/RedDict.cc
    src/RedLog.c
    src/jni/ByteBuffer.cc
    src/jni/Util.cc
    src/jni/ArrayList.cc
    src/jni/Bundle.cc
    src/jni/Env.cc
)

add_library(redbase SHARED ${SRC_LIST})

set(CMAKE_SYSTEM_VERSION 21)
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})

find_library(
    log-lib
    log
)

target_link_libraries(
    redbase
    ${log-lib}
)
