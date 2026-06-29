cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "gamepads")
project(${PROJECT_NAME} LANGUAGES C CXX)

find_package(PkgConfig)
pkg_check_modules(PMLOG REQUIRED PmLogLib)
pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
pkg_check_modules(LIBUDEV REQUIRED libudev)

if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm" AND
    NOT "${CMAKE_CXX_COMPILER}" MATCHES "aarch64" )
  ADD_COMPILE_OPTIONS(-mfpu=neon -mfloat-abi=softfp)
  add_definitions (-D__ARM_NEON)
endif()
add_definitions (-DOS_WEBOS)

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "gamepads_webos_plugin")

add_library(${PLUGIN_NAME} SHARED
  "gamepads_webos_plugin.cc"
  "gamepad.cc"
  "connection_listener.cc"
  "utils.cc"
  "gamepad_blocklist.cc"
  "gamepad_id_list.cc"
  "gamepad_standard_mappings.cc"
  "gamepad_standard_mappings_linux.cc"
  "log.cc"
)

apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(${PLUGIN_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${PMLOG_INCLUDE_DIRS}
   ${PBNJSON_CPP_INCLUDE_DIRS}
   ${LIBUDEV_INCLUDE_DIRS}
)

target_link_libraries(
  ${PLUGIN_NAME} PRIVATE flutter
  ${PMLOG_LIBRARIES}
  ${PBNJSON_CPP_LIBRARIES}
  ${LIBUDEV_LIBRARIES}
  atomic
)

# List of absolute paths to libraries that should be bundled with the plugin
set(gamepads_bundled_libraries
  ""
  PARENT_SCOPE
)
