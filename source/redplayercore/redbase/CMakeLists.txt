cmake_minimum_required(VERSION 3.10.2)

project(redbase)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(SRC_LIST src/RedDict.cc src/RedLog.cc)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
  set(CMAKE_SYSTEM_VERSION 21)
  set(SRC_LIST
      ${SRC_LIST}
      src/jni/ByteBuffer.cc
      src/jni/Util.cc
      src/jni/ArrayList.cc
      src/jni/Bundle.cc
      src/jni/Env.cc
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_definitions(-D__HARMONY__)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(redbase SHARED ${SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  find_library(log-lib log)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  find_library(log-lib hilog_ndk.z)
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

target_link_libraries(redbase ${log-lib})
