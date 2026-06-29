cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "firebase_auth_webos")
project(${PROJECT_NAME} LANGUAGES CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "firebase_auth_webos_plugin")

add_library(${PLUGIN_NAME} SHARED
  firebase_auth_webos_plugin.cc
  firebase_auth_webos_plugin.h
  messages.g.cc
)
apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET default)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"  # Public API headers
)
target_include_directories(${PLUGIN_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"          # Implementation files
)
target_link_libraries(${PLUGIN_NAME} PRIVATE
  flutter
  firebase_auth
  firebase_app
  firebase_core_webos_plugin
)

# List of absolute paths to libraries that should be bundled with the plugin
set(firebase_auth_webos_bundled_libraries
  ""
  PARENT_SCOPE
)
