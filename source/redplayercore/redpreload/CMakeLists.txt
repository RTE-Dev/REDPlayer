cmake_minimum_required(VERSION 3.10.2)

project(redpreload)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")
set(REDDOWNLOAD_DIR "${ROOT_DIR}/../reddownload")
set(REDSTRATEGYCENTER_DIR "${ROOT_DIR}/../redstrategycenter")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(TARGET_PLATFORM android)
  set(CMAKE_SYSTEM_VERSION 21)
  set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

set(EXTRA_REDBASE_DIR "${REDBASE_DIR}")

include_directories(
  "${REDBASE_DIR}/include"
  "${REDDOWNLOAD_DIR}"
  "${REDSTRATEGYCENTER_DIR}"
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(SRC_LIST android/redpreload_jni.cpp)
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

add_library(redpreload SHARED ${SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  find_library(log-lib log)
  target_link_libraries(
    redpreload
    ${log-lib}
    redbase
    reddownload
    redstrategycenter
  )
endif()
