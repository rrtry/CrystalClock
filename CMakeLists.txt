cmake_minimum_required(VERSION 3.11)
project(CrystalClock)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/../bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../bin/Release")
set(CMAKE_CXX_STANDARD 11)

set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

set(LWP_GIT_TAG fix/window-behavior CACHE STRING "Git tag/branch for LuminWallpaper")
FetchContent_Declare(
  lumin
  GIT_REPOSITORY https://github.com/rrtry/LuminWallpaper.git
  GIT_TAG ${LWP_GIT_TAG}
)
FetchContent_MakeAvailable(lumin)

set(BASE_SOURCES src/config.cpp src/config.h src/clock.cpp src/clock.h src/timeinfo.cpp src/timeinfo.h)
if (${PLATFORM} STREQUAL "Desktop")
    if (DESKTOP_WALLPAPER)
      add_executable(${PROJECT_NAME} src/lumin_main.cpp ${BASE_SOURCES})
      target_compile_definitions(
        ${PROJECT_NAME}
          PRIVATE
          WALLPAPER
      )
    else()
      add_executable(${PROJECT_NAME} src/main.cpp ${BASE_SOURCES})
    endif()
    if (MSVC)
      target_compile_definitions(${PROJECT_NAME}
          PRIVATE
            UNICODE
            _UNICODE
      )
    endif()
    if (DESKTOP_WALLPAPER)
      target_link_libraries(${PROJECT_NAME} raylib lumin)
    else()
      target_link_libraries(${PROJECT_NAME} raylib)
    endif()
endif()

if (${PLATFORM} STREQUAL "Web")

    add_executable(${PROJECT_NAME} src/emscripten_main.cpp ${BASE_SOURCES})
    target_link_libraries(${PROJECT_NAME} raylib)
    
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -s USE_GLFW=3 \
    -s ASSERTIONS=1 \
    -s WASM=1 \
    -s ASYNCIFY \
    -s GL_ENABLE_GET_PROC_ADDRESS=1 \
    -sEXPORTED_RUNTIME_METHODS=HEAPF32 \
    --preload-file ${CMAKE_SOURCE_DIR}/resources@/resources")
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()