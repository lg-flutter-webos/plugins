cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "audioplayers")
project(${PROJECT_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -fvisibility=hidden")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -v -fvisibility=hidden")

find_package(PkgConfig)
pkg_check_modules(PMLOG REQUIRED PmLogLib)
pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(NDLMEDIA REQUIRED ndlmedia)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(OGG REQUIRED ogg)
pkg_check_modules(OPUS REQUIRED opus)


if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm" AND
    NOT "${CMAKE_CXX_COMPILER}" MATCHES "aarch64" )
  add_definitions (-D__ARM_NEON)
endif()

set(PLUGIN_NAME "audioplayers_soloud_plugin")

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")

## Add SoLoud custom cmake files
message("**************** SOLOUD CONFIGURE.CMAKE")
include (Configure.cmake)
message("**************** SOLOUD SRC.CMAKE START")
if (UNIX AND NOT WIN32 AND NOT APPLE)
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
    set (LIB_POSTFIX "64" CACHE STRING "suffix for 32/64 dir placement")
    mark_as_advanced (LIB_POSTFIX)
  endif ()
endif ()
if (MSVC)
  add_definitions (-D_CRT_SECURE_NO_WARNINGS)
endif()
if (NOT DEFINED LIB_POSTFIX)
  set (LIB_POSTFIX "")
endif ()

include_directories(${SRC_DIR}/soloud/include)
include_directories(${SRC_DIR}/soloud/src)
include_directories(${SRC_DIR})
include (src.cmake)
message("**************** SOLOUD SRC.CMAKE END  ${TARGET_NAME}")




list(APPEND PLUGIN_SOURCES
  "audioplayers_soloud_plugin.cc"
  "audio_player.cc"
  "http.cc"
  "log.cc"
  "wave_generator.cc"
  "dummy_player.cc"
  "webos_player.cc"
  "${SRC_DIR}/common.cpp"
  "${SRC_DIR}/player.cpp"
  "${SRC_DIR}/analyzer.cpp"
  "${SRC_DIR}/synth/basic_wave.cpp"
  #"${SRC_DIR}/waveform/waveform.cpp"
  "${SRC_DIR}/audiobuffer/audiobuffer.cpp"
  "${SRC_DIR}/filters/filters.cpp"
  "${SRC_DIR}/filters/pitch_shift_filter.cpp"
  "${SRC_DIR}/filters/smbPitchShift.cpp"
  "${SRC_DIR}/filters/limiter.cpp"
  "${SRC_DIR}/filters/compressor.cpp"
  ${TARGET_SOURCES}
)

add_library(${PLUGIN_NAME} SHARED
  ${PLUGIN_SOURCES}
)
apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)



target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)
target_include_directories(${PLUGIN_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PLUGIN_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${ALSA_INCLUDE_DIRS}
   ${PMLOG_INCLUDE_DIRS}
   ${PBNJSON_CPP_INCLUDE_DIRS}
   ${GLIB_INCLUDE_DIRS}
   ${CURL_INCLUDE_DIRS}
   ${NDLMEDIA_INCLUDE_DIRS}
   ${SRC_DIR}
   ${OPUS_INCLUDE_DIRS}
   ${OGG_INCLUDE_DIRS}
)

target_link_libraries(
  ${PLUGIN_NAME} PRIVATE flutter
  ${ALSA_LIBRARIES}
  ${PMLOG_LIBRARIES}
  ${PBNJSON_CPP_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${CURL_LIBRARIES}
  ${NDLMEDIA_LIBRARIES}
  ${OPUS_LIBRARIES}
  ${OGG_LIBRARIES}
  atomic
)

if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm" AND
    NOT "${CMAKE_CXX_COMPILER}" MATCHES "aarch64" )
  target_compile_options(${PLUGIN_NAME} PRIVATE -fPIC -mfpu=neon -mfloat-abi=softfp)
else()
  target_compile_options(${PLUGIN_NAME} PRIVATE -fPIC)
endif()
target_compile_options(${PLUGIN_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wno-vla -Wno-error>)
target_compile_options(${PLUGIN_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wall -Wno-vla -Wno-error>)

# List of absolute paths to libraries that should be bundled with the plugin
set(audioplayers_bundled_libraries
  ""
  PARENT_SCOPE
)
