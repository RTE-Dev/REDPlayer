cmake_minimum_required(VERSION 3.10.2)

project(redrender)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")
set(EXTRA_REDBASE_DIR "${REDBASE_DIR}")

set(SRC_LIST
    video/video_renderer_factory.cpp
    video/video_filter.cpp
    video/video_renderer_info.cpp
    video/video_renderer.cpp
    video/opengl/opengl_video_renderer.cpp
    video/opengl/input/opengl_video_input.cpp
    video/opengl/output/opengl_video_output.cpp
    video/opengl/common/gl_program.cpp
    video/opengl/common/context.cpp
    video/opengl/common/framebuffer.cpp
    video/opengl/common/gl_util.cpp
    video/opengl/filter/filter_shader.cpp
    video/opengl/filter/opengl_filter_base.cpp
    video/opengl/filter/opengl_device_filter.cpp
    audio/audio_render_factory.cc
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  add_definitions(-DPLATFORM_ANDROID)
  set(TARGET_PLATFORM android)
  set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
  set(SRC_LIST
      ${SRC_LIST}
      video/opengl/android/egl_context.cpp
      audio/android/audio_track_render.cc
      audio/android/jni/audio_track_jni.cc
      video/mediacodec/mediacodec_video_renderer.cpp
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_definitions(-D__HARMONY__)
  set(TARGET_PLATFORM harmony)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument"
  )
  set(SRC_LIST
      ${SRC_LIST}
      video/opengl/harmony/egl_context.cc
      video/harmony/harmony_video_renderer.cc
      audio/harmony/harmony_audio_render.cc
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

include_directories(
  "${EXTRA_REDBASE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_library(redrender SHARED ${SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(CMAKE_SYSTEM_VERSION 21)
  target_link_libraries(
    redrender
    redbase
    EGL
    GLESv2
    android
    jnigraphics
    log
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  target_link_libraries(
    redrender
    redbase
    native_media_core
    native_window
    ohaudio
    EGL
    GLESv3
  )
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()
