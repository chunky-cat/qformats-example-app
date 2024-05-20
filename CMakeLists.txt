cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_VERBOSE_MAKEFILE 1)

project(
    qmap_viewer
    VERSION 1.0
    LANGUAGES CXX
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DMACOSX)
endif()

find_package(OpenGL REQUIRED) # error

add_executable( ${PROJECT_NAME}
    src/thirdparty/xatlas/xatlas.cpp
    src/main.cpp
    src/QuakeMap.cpp
    src/Scene.cpp
)

add_subdirectory(deps/qformats)

# Dependencies
set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()


target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARY}
    qformats
    raylib
)

add_custom_target(
    run-qmap-viewer
    COMMAND ${PROJECT_NAME}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
