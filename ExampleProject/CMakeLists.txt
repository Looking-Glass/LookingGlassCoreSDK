# Projects Settings
cmake_minimum_required (VERSION 3.1)
project (holoplay-core-example)

# common ifdefs
if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(HOLOPLAY_CORE_BASE_PATH "${PROJECT_SOURCE_DIR}/../HoloPlayCore")

# The main executable
add_executable(main
  src/HoloPlayContext.hpp
  src/HoloPlayContext.cpp
  src/SampleScene.hpp
  src/SampleScene.cpp
  src/glError.hpp
  src/glError.cpp
  src/main.cpp
  src/Shader.hpp
  src/Shader.cpp
)

set_property(TARGET main PROPERTY CXX_STANDARD 11)
target_compile_options(main PRIVATE -Wall)

# glfw
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE glfw)

# glew
add_definitions(-DGLEW_STATIC)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE libglew_static)

# glm
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE glm)

if (LINUX)
  set(DLL_DIR "linux")
endif(LINUX)

if(WIN32)
  set(DLL_DIR "Win64")
  if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(DLL_DIR "Win32")
  endif()
  set(HOLOPLAY_CORE_DLL_LOCATION "${HOLOPLAY_CORE_BASE_PATH}/dylib/${DLL_DIR}/HoloPlayCore.dll")
  add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HOLOPLAY_CORE_DLL_LOCATION}" $<TARGET_FILE_DIR:main>)
endif(WIN32)

if(APPLE)
  set(DLL_DIR "macos")
endif(APPLE)

# holoplaycore
target_include_directories(main PRIVATE "${HOLOPLAY_CORE_BASE_PATH}/include")
if(LINUX)
  target_link_directories(main PRIVATE "${HOLOPLAY_CORE_BASE_PATH}/dylib/${DLL_DIR}")
  target_link_libraries(main PRIVATE LookingGlassCore)
else()
  find_library(HOLOPLAY_CORE_LOCATION HoloPlayCore PATHS "${HOLOPLAY_CORE_BASE_PATH}/dylib" PATH_SUFFIXES ${DLL_DIR})
endif(LINUX)
