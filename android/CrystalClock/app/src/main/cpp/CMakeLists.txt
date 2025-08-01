cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(APP_LIB_NAME crystalclock)
set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)

project("${APP_LIB_NAME}")
add_link_options("LINKER:--build-id=none")

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raylib)

get_filename_component(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../../src" ABSOLUTE)
file(GLOB_RECURSE ANDROID_PROJ_SOURCES "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")
list(FILTER ANDROID_PROJ_SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/deps/.*")

file(GLOB DESKTOP_PROJ_SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.h")
list(FILTER DESKTOP_PROJ_SOURCES EXCLUDE REGEX ".*main.*")

set(SOURCES ${DESKTOP_PROJ_SOURCES} ${ANDROID_PROJ_SOURCES})

include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)
list(APPEND SOURCES ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_library(${APP_LIB_NAME} SHARED ${SOURCES})
target_compile_definitions(${APP_LIB_NAME} PRIVATE PLATFORM_ANDROID)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${APP_LIB_NAME} PRIVATE _DEBUG DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-limit-debug-info")
    set(CMAKE_C_FLAGS_DEBUG "-g -fno-limit-debug-info")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

target_include_directories(${APP_LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/deps/raylib")
target_link_libraries(${APP_LIB_NAME} raylib)