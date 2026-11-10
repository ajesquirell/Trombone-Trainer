include(FetchContent)
FetchContent_Declare(pge3_source
  GIT_REPOSITORY https://github.com/OneLoneCoder/olcPixelGameEngine3.git
  GIT_TAG ea99cd5a8abeb3972cc04235aa3b866713dc9fa8
  SOURCE_SUBDIR _trainer_source_only)
FetchContent_MakeAvailable(pge3_source)
add_library(pge3 INTERFACE)
target_compile_features(pge3 INTERFACE cxx_std_20)
# Generate a private copy; the dependency checkout is never modified.
file(READ "${pge3_source_SOURCE_DIR}/olcPixelGameEngine3.h" pge_header)
# Fit the entire browser viewport instead of retaining the initial canvas aspect.
# The application recalculates its logical screen geometry after every resize.
set(pge_resize "double height = parentWidth / aspect;")
string(FIND "${pge_header}" "${pge_resize}" pge_resize_match)
if(pge_resize_match EQUAL -1)
  message(FATAL_ERROR "PGE3 viewport compatibility patch no longer matches")
endif()
string(REPLACE "${pge_resize}" "double height = parentHeight;" pge_header "${pge_header}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pge3")
# The packed font contains literal @name@ sequences, so template expansion
# corrupts it. COPYONLY preserves bytes and avoids touching unchanged output.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pge3/olcPixelGameEngine3.h.in" "${pge_header}")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/pge3/olcPixelGameEngine3.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pge3/olcPixelGameEngine3.h" COPYONLY)
target_include_directories(pge3 SYSTEM INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/pge3")
if(EMSCRIPTEN)
  target_compile_options(pge3 INTERFACE -sUSE_LIBPNG=1)
  target_link_options(pge3 INTERFACE -sASYNCIFY -sALLOW_MEMORY_GROWTH=1
    -sSTACK_SIZE=4194304 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
    -sEXPORTED_RUNTIME_METHODS=HEAPF32 -sUSE_LIBPNG=1)
elseif(APPLE)
  enable_language(OBJCXX)
  target_link_libraries(pge3 INTERFACE "-framework Metal" "-framework MetalKit"
    "-framework AppKit" "-framework QuartzCore" "-framework OpenGL" "-framework Foundation")
elseif(WIN32)
  target_link_libraries(pge3 INTERFACE user32 gdi32 dwmapi gdiplus shlwapi opengl32)
else()
  find_package(PNG REQUIRED)
  find_package(OpenGL REQUIRED)
  find_package(X11 REQUIRED COMPONENTS Xi)
  find_package(Threads REQUIRED)
  target_link_libraries(pge3 INTERFACE PNG::PNG OpenGL::GL X11::X11 X11::Xi Threads::Threads)
endif()
function(trainer_platform target)
  target_link_libraries(${target} PRIVATE pge3)
  if(APPLE)
    get_target_property(sources ${target} SOURCES)
    set_source_files_properties(${sources} PROPERTIES LANGUAGE OBJCXX)
  endif()
  if(EMSCRIPTEN)
    set_target_properties(${target} PROPERTIES SUFFIX ".html")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${PROJECT_SOURCE_DIR}/cmake/shell.html")
    target_link_options(${target} PRIVATE "--shell-file" "${PROJECT_SOURCE_DIR}/cmake/shell.html")
  endif()
endfunction()
