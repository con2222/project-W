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

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.14
)

FetchContent_MakeAvailable(SDL3)

find_package(Threads REQUIRED)
find_package(Dawn REQUIRED)

add_library(webgpu ALIAS dawn::webgpu_dawn)

add_subdirectory(vendor/c2-core-lib)

add_subdirectory(vendor/sdl3webgpu)
target_compile_definitions(sdl3webgpu PRIVATE WEBGPU_BACKEND_DAWN)

add_subdirectory(vendor/imgui)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

set(SOURCES 
    src/main.cpp
    src/webgpu_context.cpp
    src/miniaudio.c
    src/window.cpp
    src/webgpu_utils.cpp
    src/render.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC dawn::webgpu_dawn SDL3::SDL3 C2Core sdl3webgpu imgui)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)