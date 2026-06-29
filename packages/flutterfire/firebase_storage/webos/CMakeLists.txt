cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "firebase_storage_webos")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_NAME "firebase_storage_webos_plugin")

add_library(${PLUGIN_NAME} SHARED
  "firebase_storage_webos_plugin.cc"
  "messages.g.cc"
)
apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET default)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PLUGIN_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  firebase_storage
  firebase_app
  firebase_core_webos_plugin   # provides FirebaseAppHolder
  atomic
)

set(firebase_storage_webos_bundled_libraries
  ""
  PARENT_SCOPE
)
