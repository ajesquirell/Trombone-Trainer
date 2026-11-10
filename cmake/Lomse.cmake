include(FetchContent)

# The 0.30.0 release. Keep the renderer and the bundled fonts at one revision.
FetchContent_Declare(lomse
  GIT_REPOSITORY https://github.com/lenmus/lomse.git
  GIT_TAG 9f774d7a3a3904c89b7ebcc8fbada085a9f3d687
  GIT_PROGRESS TRUE)

set(LOMSE_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
set(LOMSE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(LOMSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LOMSE_RUN_TESTS OFF CACHE BOOL "" FORCE)
set(LOMSE_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(LOMSE_ENABLE_THREADS OFF CACHE BOOL "" FORCE)
set(LOMSE_ENABLE_FONTCONFIG OFF CACHE BOOL "" FORCE)
set(LOMSE_ENABLE_PNG OFF CACHE BOOL "" FORCE)
set(LOMSE_ENABLE_COMPRESSION OFF CACHE BOOL "" FORCE)
set(LOMSE_DOWNLOAD_BRAVURA_FONT OFF CACHE BOOL "" FORCE)
set(LOMSE_INSTALL_BRAVURA_FONT OFF CACHE BOOL "" FORCE)
set(LOMSE_USING_EMSCRIPTEN "${EMSCRIPTEN}" CACHE BOOL "" FORCE)
FetchContent_GetProperties(lomse)
if(NOT lomse_POPULATED)
  FetchContent_Populate(lomse)
endif()

# This release assumes it is the top-level CMake project in its build-version
# command. Work on a build-local copy so local source overrides stay untouched.
set(TRAINER_LOMSE_UPSTREAM_DIR "${lomse_SOURCE_DIR}")
set(lomse_SOURCE_DIR "${CMAKE_BINARY_DIR}/lomse-patched")
file(COPY "${TRAINER_LOMSE_UPSTREAM_DIR}/" DESTINATION "${lomse_SOURCE_DIR}"
  PATTERN ".git" EXCLUDE)
file(READ "${lomse_SOURCE_DIR}/CMakeLists.txt" lomse_cmake_source)
string(REPLACE
  [=[-D LOMSE_ROOT_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/build-version.cmake]=]
  [=[-D LOMSE_ROOT_DIR=${LOMSE_ROOT_DIR} -P ${LOMSE_ROOT_DIR}/build-version.cmake]=]
  lomse_cmake_source "${lomse_cmake_source}")
file(WRITE "${lomse_SOURCE_DIR}/CMakeLists.txt" "${lomse_cmake_source}")

# Newer FreeType uses unsigned char* for outline tags; older versions use char*.
# Follow the installed API's type in our private copy rather than casting it.
set(lomse_freetype_file "${lomse_SOURCE_DIR}/src/render/lomse_font_freetype.cpp")
file(READ "${lomse_freetype_file}" lomse_freetype_source)
set(lomse_tags_old "char*       tags;")
set(lomse_tags_new "decltype(outline.tags) tags;")
string(FIND "${lomse_freetype_source}" "${lomse_tags_old}" lomse_tags_match)
if(lomse_tags_match EQUAL -1)
  string(FIND "${lomse_freetype_source}" "${lomse_tags_new}" lomse_tags_match)
  if(lomse_tags_match EQUAL -1)
    message(FATAL_ERROR "Lomse FreeType compatibility patch no longer matches")
  endif()
endif()
string(REPLACE "${lomse_tags_old}" "${lomse_tags_new}"
  lomse_freetype_source "${lomse_freetype_source}")
file(WRITE "${lomse_freetype_file}" "${lomse_freetype_source}")

# This release relies on a transitive <limits> include that GCC 13 omits.
set(lomse_wedge_file "${lomse_SOURCE_DIR}/src/graphic_model/engravers/lomse_wedge_engraver.cpp")
file(READ "${lomse_wedge_file}" lomse_wedge_source)
string(FIND "${lomse_wedge_source}" "#include <limits>" lomse_limits_match)
if(lomse_limits_match EQUAL -1)
  file(WRITE "${lomse_wedge_file}" "#include <limits>\n${lomse_wedge_source}")
endif()
add_subdirectory("${lomse_SOURCE_DIR}" "${lomse_BINARY_DIR}" EXCLUDE_FROM_ALL)

# Upstream does not export its header search paths, and build-version.cmake
# generates lomse_version.h in the top-level build directory.
target_include_directories(lomse SYSTEM PUBLIC
  "${lomse_SOURCE_DIR}/include"
  "${lomse_SOURCE_DIR}/src/agg/include"
  "${lomse_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}")
if(EMSCRIPTEN)
  # These ports must be available to the final link as well as Lomse's compile.
  target_compile_options(lomse PUBLIC -fexceptions -sUSE_FREETYPE=1)
  target_link_options(lomse INTERFACE
    -fexceptions -sUSE_FREETYPE=1 -sUSE_ZLIB=1 -sUSE_LIBPNG=1)
else()
  # Lomse's public view headers include FreeType headers as well.
  find_package(Freetype REQUIRED)
  # Match the plain signature used by Lomse 0.30.0 (transitive by default).
  target_link_libraries(lomse Freetype::Freetype)
endif()

set(TRAINER_ASSET_DIR "${CMAKE_BINARY_DIR}/assets")
file(MAKE_DIRECTORY "${TRAINER_ASSET_DIR}/fonts")
foreach(font IN ITEMS Bravura.otf LiberationSerif-Regular.ttf)
  if(NOT EXISTS "${lomse_SOURCE_DIR}/fonts/${font}")
    message(FATAL_ERROR
      "Lomse's bundled ${font} is missing. Use the pinned Lomse source tree, including fonts/.")
  endif()
endforeach()
# Preserve the upstream font licenses and all style variants along with fonts.
file(COPY "${lomse_SOURCE_DIR}/fonts/" DESTINATION "${TRAINER_ASSET_DIR}/fonts")
if(EMSCRIPTEN)
  set(TRAINER_FONT_DIR "/assets/fonts")
else()
  set(TRAINER_FONT_DIR "${TRAINER_ASSET_DIR}/fonts")
endif()
