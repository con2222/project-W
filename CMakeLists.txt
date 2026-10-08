cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(audio-player)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build static SDL library instead of shared" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build static SDL" FORCE)
set(PROJECT_NAME audio-player)

option(USE_WAYLAND "Build with Wayland support" OFF)
option(USE_X11 "Build with X11 support" OFF)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.14
)

FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1
)

FetchContent_MakeAvailable(Catch2)

find_package(Threads REQUIRED)
find_package(Dawn REQUIRED)

add_library(webgpu ALIAS dawn::webgpu_dawn)

add_subdirectory(vendor/c2-core-lib)

add_subdirectory(vendor/sdl3webgpu)
target_compile_definitions(sdl3webgpu PRIVATE WEBGPU_BACKEND_DAWN)

add_subdirectory(vendor/imgui)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/taglib)

add_subdirectory(vendor/miniaudio)

set(CORE_SOURCES 
    src/webgpu_context.cpp
    src/window.cpp
    src/webgpu_utils.cpp
    src/render.cpp
    src/audio.cpp
    src/music_player_ui.cpp
    src/player.cpp
    src/command.cpp
    src/imgui_layer.cpp
    src/app.cpp
)

add_library(audio-core STATIC ${CORE_SOURCES})

target_include_directories(audio-core
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/taglib/taglib/toolkit
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/taglib/taglib
)

target_link_libraries(audio-core PUBLIC dawn::webgpu_dawn SDL3::SDL3 C2Core sdl3webgpu imgui tag miniaudio)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE audio-core)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)


add_executable(audio-tests tests/test_shuffle.cpp)
target_link_libraries(audio-tests PRIVATE audio-core Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(audio-tests)