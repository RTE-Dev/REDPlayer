cmake_minimum_required(VERSION 3.10.2)

project(redpreload)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

message("CMAKE_SYSTEM NAME: ${CMAKE_SYSTEM_NAME}")
message("CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_PLATFORM android)
else()
    message(FATAL_ERROR "This CMake script currently only supports Android!")
endif()

set(EXTRA_REDBASE_DIR "${REDBASE_DIR}")
set(EXTRA_REDDOWNLOAD_DIR "${EXTRA_DIR}/reddownload/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_REDSTRATEGYCENTER_DIR "${EXTRA_DIR}/redstrategycenter/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")

include_directories(
    "${EXTRA_REDBASE_DIR}/include"
    "${EXTRA_REDDOWNLOAD_DIR}/include"
    "${EXTRA_REDSTRATEGYCENTER_DIR}/include"
    ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
    "${EXTRA_REDDOWNLOAD_DIR}/lib"
    "${EXTRA_REDSTRATEGYCENTER_DIR}/lib"
)

set(SRC_LIST redpreload_jni.cpp)

message(STATUS "SRC_LIST= ${SRC_LIST}")
message(STATUS "EXTRA_REDBASE_DIR= ${EXTRA_REDBASE_DIR}")
message(STATUS "LIB_INSTALL_DIR= ${LIB_INSTALL_DIR}")

add_library(redpreload SHARED ${SRC_LIST})

set(CMAKE_SYSTEM_VERSION 21)
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})

target_link_libraries(
    redpreload
    redbase
    reddownload
    redstrategycenter
)
