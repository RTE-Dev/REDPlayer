cmake_minimum_required(VERSION 3.10.2)
project(redstrategycenter)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")
set(JSON_DIR "${EXTRA_DIR}/nlohmann-json")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(TARGET_PLATFORM android)
  set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
  set(CMAKE_SYSTEM_VERSION 21)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_definitions(-D__HARMONY__)
  set(TARGET_PLATFORM harmony)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument"
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

include_directories(
  ${JSON_DIR} "${REDBASE_DIR}/include" ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB_RECURSE SRC_LIST "*.cc")

add_library(redstrategycenter SHARED ${SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  find_library(log-lib log)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  find_library(log-lib hilog_ndk.z)
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

target_link_libraries(redstrategycenter ${log-lib} redbase)
