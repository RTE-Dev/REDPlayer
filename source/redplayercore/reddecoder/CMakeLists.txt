cmake_minimum_required(VERSION 3.10.2)

project(reddecoder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")
set(EXTRA_REDBASE_DIR "${REDBASE_DIR}")

set(SRC_LIST
    common/buffer.cpp
    video/video_codec_info.cpp
    video/video_decoder/video_decoder.cpp
    video/video_decoder/video_decoder_factory.cpp
    video/video_decoder/ffmpeg_video_decoder.cpp
    audio/audio_codec_info.cpp
    audio/audio_decoder/audio_decoder.cpp
    audio/audio_decoder/audio_decoder_factory.cpp
    audio/audio_decoder/ffmpeg_audio_decoder.cpp
    video/video_common/format_convert_helper.cpp
    video/video_common/h264_parser.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(TARGET_PLATFORM android)
  set(CMAKE_SYSTEM_VERSION 21)
  set(EXTRA_FFMPEG_DIR
      "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}"
  )
  set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
  set(SRC_LIST ${SRC_LIST}
               video/video_decoder/android/ndk_mediacodec_video_decoder.cpp
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_definitions(-D__HARMONY__)
  set(TARGET_PLATFORM harmony)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument"
  )
  set(EXTRA_FFMPEG_DIR "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${OHOS_ARCH}")
  set(SRC_LIST ${SRC_LIST}
               video/video_decoder/harmony/harmony_video_decoder.cpp
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

include_directories(
  "${EXTRA_FFMPEG_DIR}/include" "${EXTRA_REDBASE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

link_directories("${EXTRA_FFMPEG_DIR}")

add_library(reddecoder SHARED ${SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  target_link_libraries(
    reddecoder
    mediandk
    android
    redbase
    ffmpeg
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  target_link_libraries(
    reddecoder
    ffmpeg
    redbase
    native_media_vdec
    native_media_core
    native_media_codecbase
    native_window
  )
endif()
