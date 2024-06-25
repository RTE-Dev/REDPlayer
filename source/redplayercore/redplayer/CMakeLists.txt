cmake_minimum_required(VERSION 3.10.2)

project(redplayer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")
set(REDSOURCE_DIR "${ROOT_DIR}/../redsource")
set(REDDOWNLOAD_DIR "${ROOT_DIR}/../reddownload")
set(REDSTRATEGYCENTER_DIR "${ROOT_DIR}/../redstrategycenter")
set(REDPLAYERCORE_DIR "${ROOT_DIR}/../")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(TARGET_PLATFORM android)
  set(CMAKE_SYSTEM_VERSION 21)
  set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
  set(EXTRA_FFMPEG_DIR
      "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${CMAKE_ANDROID_ARCH_ABI}"
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_definitions(-D__HARMONY__)
  set(TARGET_PLATFORM harmony)
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -Wno-c99-designator"
  )
  set(EXTRA_FFMPEG_DIR "${EXTRA_DIR}/ffmpeg/${TARGET_PLATFORM}/${OHOS_ARCH}")
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

include_directories(
  "${EXTRA_FFMPEG_DIR}/include"
  "${REDBASE_DIR}/include"
  "${REDDOWNLOAD_DIR}/"
  "${REDSOURCE_DIR}"
  "${REDSTRATEGYCENTER_DIR}"
  "${REDPLAYERCORE_DIR}"
  ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories("${EXTRA_FFMPEG_DIR}")

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
    RedCore/module/renderHal/SoundTouchHal.cpp
    RedCore/module/renderHal/soundTouch/AAFilter.cpp
    RedCore/module/renderHal/soundTouch/BPMDetect.cpp
    RedCore/module/renderHal/soundTouch/cpu_detect_x86.cpp
    RedCore/module/renderHal/soundTouch/FIFOSampleBuffer.cpp
    RedCore/module/renderHal/soundTouch/FIRFilter.cpp
    RedCore/module/renderHal/soundTouch/InterpolateCubic.cpp
    RedCore/module/renderHal/soundTouch/InterpolateLinear.cpp
    RedCore/module/renderHal/soundTouch/InterpolateShannon.cpp
    RedCore/module/renderHal/soundTouch/mmx_optimized.cpp
    RedCore/module/renderHal/soundTouch/PeakFinder.cpp
    RedCore/module/renderHal/soundTouch/RateTransposer.cpp
    RedCore/module/renderHal/soundTouch/SoundTouch.cpp
    RedCore/module/renderHal/soundTouch/sse_optimized.cpp
    RedCore/module/renderHal/soundTouch/TDStretch.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(SRC_LIST ${SRC_LIST} android/redplayer_jni.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  set(SRC_LIST
      ${SRC_LIST}
      harmony/redplayer_callarktslog.cpp
      harmony/redplayer_event_dispatcher.cpp
      harmony/redplayer_logcallback.cpp
      harmony/preload_event_dispatcher.cpp
      harmony/redplayerproxy.cpp
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

add_library(redplayer SHARED ${SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  find_library(log-lib log)
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
  )
  set(LIBRARY_OUTPUT_DIRECTORY
      "${ROOT_DIR}/../../android/OpenRedPlayerCore/libs/${CMAKE_ANDROID_ARCH_ABI}"
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  find_library(EGL-lib EGL)
  find_library(GLES-lib GLESv3)
  find_library(libace-lib ace_ndk.z)
  target_link_libraries(
    redplayer
    ace_napi.z
    hilog_ndk.z
    native_media_core
    native_media_adec
    native_media_vdec
    native_media_avdemuxer
    native_media_avsource
    native_media_codecbase
    z
    c
    m
    ${EGL-lib}
    ${GLES-lib}
    ${libace-lib}
    redbase
    ffmpeg
    reddecoder
    redrender
    reddownload
    redsource
    redstrategycenter
    uv
  )
  set(LIBRARY_OUTPUT_DIRECTORY
      "${ROOT_DIR}/../../harmony/redplayerproxy/libs/${OHOS_ARCH}"
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  add_custom_command(
    TARGET redplayer
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${EXTRA_FFMPEG_DIR}/libffmpeg.so
            ${LIBRARY_OUTPUT_DIRECTORY}
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_custom_command(
    TARGET redplayer
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${EXTRA_FFMPEG_DIR}/libffmpeg.so
            ${LIBRARY_OUTPUT_DIRECTORY}
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()
