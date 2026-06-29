cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME "webos_image_texture")
project(${PROJECT_NAME} LANGUAGES C CXX)

find_package(PkgConfig)
pkg_check_modules(PMLOG REQUIRED PmLogLib)
pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(WEBP REQUIRED libwebp)
pkg_check_modules(WEBPDEMUX REQUIRED libwebpdemux)

if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm" AND
    NOT "${CMAKE_CXX_COMPILER}" MATCHES "aarch64" )
  ADD_COMPILE_OPTIONS(-mfpu=neon -mfloat-abi=softfp)
  add_definitions (-D__ARM_NEON)
endif()

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "webos_image_texture_plugin")

add_library(${PLUGIN_NAME} SHARED
  "image_texture_plugin.cc"
  "image_factory.cc"
  "image_provider.cc"
  "image_texture.cc"
  "image_cache.cc"
  "log.cc"
  "./third_party/etcpak/Application.cpp"
  "./third_party/etcpak/bc7enc.cpp"
  "./third_party/etcpak/bcdec.c"
  "./third_party/etcpak/Bitmap.cpp"
  "./third_party/etcpak/BitmapDownsampled.cpp"
  "./third_party/etcpak/BlockData.cpp"
  "./third_party/etcpak/ColorSpace.cpp"
  "./third_party/etcpak/DataProvider.cpp"
  "./third_party/etcpak/Debug.cpp"
  "./third_party/etcpak/Decode.cpp"
  "./third_party/etcpak/Dither.cpp"
  "./third_party/etcpak/Error.cpp"
  "./third_party/etcpak/ProcessDxtc.cpp"
  "./third_party/etcpak/ProcessRGB.cpp"
  "./third_party/etcpak/System.cpp"
  "./third_party/etcpak/Tables.cpp"
  "./third_party/etcpak/TaskDispatch.cpp"
  "./third_party/etcpak/TextureHeader.cpp"
  "./third_party/etcpak/Timing.cpp"
  "./third_party/etcpak/mmap.cpp"
  "./third_party/lunasvg/source/lunasvg.cpp"
  "./third_party/lunasvg/source/element.cpp"
  "./third_party/lunasvg/source/property.cpp"
  "./third_party/lunasvg/source/parser.cpp"
  "./third_party/lunasvg/source/layoutcontext.cpp"
  "./third_party/lunasvg/source/canvas.cpp"
  "./third_party/lunasvg/source/clippathelement.cpp"
  "./third_party/lunasvg/source/defselement.cpp"
  "./third_party/lunasvg/source/gelement.cpp"
  "./third_party/lunasvg/source/geometryelement.cpp"
  "./third_party/lunasvg/source/graphicselement.cpp"
  "./third_party/lunasvg/source/maskelement.cpp"
  "./third_party/lunasvg/source/markerelement.cpp"
  "./third_party/lunasvg/source/paintelement.cpp"
  "./third_party/lunasvg/source/stopelement.cpp"
  "./third_party/lunasvg/source/styledelement.cpp"
  "./third_party/lunasvg/source/styleelement.cpp"
  "./third_party/lunasvg/source/svgelement.cpp"
  "./third_party/lunasvg/source/symbolelement.cpp"
  "./third_party/lunasvg/source/useelement.cpp"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-paint.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-geometry.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-blend.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-rle.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-dash.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-ft-raster.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-ft-stroker.c"
  "./third_party/lunasvg/3rdparty/plutovg/plutovg-ft-math.c"
)

apply_standard_settings(${PLUGIN_NAME})
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(${PLUGIN_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/etcpak
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lunasvg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lunasvg/source
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lunasvg/3rdparty/plutovg
   ${PMLOG_INCLUDE_DIRS}
   ${PBNJSON_CPP_INCLUDE_DIRS}
   ${CURL_INCLUDE_DIRS}
   ${GLIB_INCLUDE_DIRS}
   ${SQLITE3_LIBRARY_DIRS}
   ${WEBP_INCLUDE_DIRS}
)

target_link_libraries(
  ${PLUGIN_NAME} PRIVATE flutter
  ${PMLOG_LIBRARIES}
  ${PBNJSON_CPP_LIBRARIES}
  ${CURL_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${WEBP_LIBRARIES}
  ${WEBPDEMUX_LIBRARIES}
  atomic
)

target_compile_options(${PLUGIN_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-error>)
target_compile_options(${PLUGIN_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-error>)

# Strip debug symbols in Release builds to reduce binary size
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(${PLUGIN_NAME} PRIVATE -g0)
  target_link_options(${PLUGIN_NAME} PRIVATE -s)
endif()

# List of absolute paths to libraries that should be bundled with the plugin
set(webos_image_texture_bundled_libraries
  ""
  PARENT_SCOPE
)
