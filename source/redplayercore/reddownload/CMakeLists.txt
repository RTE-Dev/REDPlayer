cmake_minimum_required(VERSION 3.10.2)

project(redplayer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_DIR "${ROOT_DIR}/../extra")
set(REDBASE_DIR "${ROOT_DIR}/../redbase")
set(JSON_DIR "${EXTRA_DIR}/nlohmann-json")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(TARGET_PLATFORM android)
  set(CURL_DIR ${EXTRA_DIR}/curl/${TARGET_PLATFORM}/${ANDROID_ABI})
  set(CARES_DIR ${EXTRA_DIR}/cares/${TARGET_PLATFORM}/${ANDROID_ABI})
  set(OPENSSL_DIR ${EXTRA_DIR}/openssl/${TARGET_PLATFORM}/${ANDROID_ABI})
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_definitions(-D__HARMONY__)
  set(TARGET_PLATFORM harmony)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument"
  )
  set(CURL_DIR ${EXTRA_DIR}/curl/${TARGET_PLATFORM}/${OHOS_ARCH})
  set(CARES_DIR ${EXTRA_DIR}/cares/${TARGET_PLATFORM}/${OHOS_ARCH})
  set(OPENSSL_DIR ${EXTRA_DIR}/openssl/${TARGET_PLATFORM}/${OHOS_ARCH})
else()
  message(
    FATAL_ERROR "This CMake script does not support ${CMAKE_SYSTEM_NAME}!"
  )
endif()

include_directories(
  ${CURL_DIR}/include
  ${CARES_DIR}/include
  ${OPENSSL_DIR}/include
  ${REDBASE_DIR}/include
  ${JSON_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

aux_source_directory(. SRC_LIST)
aux_source_directory(./dnscache DNS_SRC_LIST)
aux_source_directory(./utility UTILITY_SRC_LIST)
aux_source_directory(./wrapper WRAPPER_SRC_LIST)
list(
  APPEND
  SRC_LIST
  ${DNS_SRC_LIST}
  ${WRAPPER_SRC_LIST}
  ${UTILITY_SRC_LIST}
)

add_library(curl STATIC IMPORTED)
set_target_properties(
  curl PROPERTIES IMPORTED_LOCATION ${CURL_DIR}/lib/libcurl.a
)
add_library(cares STATIC IMPORTED)
set_target_properties(
  cares PROPERTIES IMPORTED_LOCATION ${CARES_DIR}/lib/libcares.a
)
add_library(crypto STATIC IMPORTED)
set_target_properties(
  crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_DIR}/lib/libcrypto.a
)
add_library(ssl STATIC IMPORTED)
set_target_properties(
  ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_DIR}/lib/libssl.a
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  add_library(reddownload SHARED ${SRC_LIST})
  find_library(log-lib log)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  add_library(reddownload SHARED ${SRC_LIST})
  find_library(log-lib hilog_ndk.z)
endif()

target_link_libraries(
  reddownload
  ${log-lib}
  z
  curl
  cares
  ssl
  crypto
  redbase
)
