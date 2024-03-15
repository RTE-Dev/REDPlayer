cmake_minimum_required(VERSION 3.10.2)

project(redplayer)

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
set(EXTRA_FFMPEG_DIR "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_REDRENDER_DIR "${EXTRA_DIR}/redrender/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_REDDOWNLOAD_DIR "${EXTRA_DIR}/reddownload/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_REDSOURCE_DIR "${EXTRA_DIR}/redsource/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_REDDECODER_DIR "${EXTRA_DIR}/reddecoder/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_REDSTRATEGYCENTER_DIR "${EXTRA_DIR}/redstrategycenter/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")
set(EXTRA_SOUNDTOUCH_DIR "${EXTRA_DIR}/soundtouch/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}")

include_directories("${EXTRA_FFMPEG_DIR}/include"
    "${EXTRA_REDBASE_DIR}/include"
    "${EXTRA_REDDOWNLOAD_DIR}/include"
    "${EXTRA_REDRENDER_DIR}/include"
    "${EXTRA_REDSOURCE_DIR}/include"
    "${EXTRA_REDDECODER_DIR}/include"
    "${EXTRA_REDSTRATEGYCENTER_DIR}/include"
    "${EXTRA_SOUNDTOUCH_DIR}/include"
    ${CMAKE_CURRENT_SOURCE_DIR})
link_directories("${EXTRA_FFMPEG_DIR}"
    "${EXTRA_REDRENDER_DIR}/lib"
    "${EXTRA_REDDOWNLOAD_DIR}/lib"
    "${EXTRA_REDSOURCE_DIR}/lib"
    "${EXTRA_REDDECODER_DIR}/lib"
    "${EXTRA_REDSTRATEGYCENTER_DIR}/lib"
    "${EXTRA_SOUNDTOUCH_DIR}/lib")

set(SRC_LIST
    base/RedBuffer.cpp
    base/RedClock.cpp
    base/RedConfig.cpp
    base/RedMsgQueue.cpp
    base/RedPacket.cpp
    base/RedQueue.cpp
    base/RedSampler.cpp
    Interface/RedPlayer.cpp
    RedCore/RedCore.cpp
    RedCore/module/sourcer/RedSourceController.cpp
    RedCore/module/processer/AudioProcesser.cpp
    RedCore/module/processer/VideoProcesser.cpp
    RedCore/module/renderHal/RedRenderAudioHal.cpp
    RedCore/module/renderHal/RedRenderVideoHal.cpp
    RedCore/module/sourcer/format/RedFormat.c
    RedCore/module/sourcer/format/reddownload_adapter.c
    android/redplayer_jni.cpp
    RedCore/module/renderHal/SoundTouchHal.cpp
)

message(STATUS "SRC_LIST= ${SRC_LIST}")

add_library(redplayer SHARED ${SRC_LIST})

set(CMAKE_SYSTEM_VERSION 21)
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})

find_library(
    log-lib
    log
)

target_link_libraries(
    redplayer
    ${log-lib}
    redbase
    redrender
    reddownload
    redsource
    reddecoder
    redstrategycenter
    ffmpeg
    mediandk
    android
    soundtouch
)

set(LIBRARY_OUTPUT_DIRECTORY "${ROOT_DIR}/../../android/OpenRedPlayerCore/libs/${CMAKE_ANDROID_ARCH_ABI}")

file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_FFMPEG_DIR}/libffmpeg.so
                   ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_SOUNDTOUCH_DIR}/lib/libsoundtouch.so
                   ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_REDRENDER_DIR}/lib/libredrender.so
                   ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_REDDOWNLOAD_DIR}/lib/libreddownload.so
                   ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_REDSOURCE_DIR}/lib/libredsource.so
                   ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_REDDECODER_DIR}/lib/libreddecoder.so
                   ${LIBRARY_OUTPUT_DIRECTORY})

add_custom_command(TARGET redplayer PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${EXTRA_REDSTRATEGYCENTER_DIR}/lib/libredstrategycenter.so
                   ${LIBRARY_OUTPUT_DIRECTORY})