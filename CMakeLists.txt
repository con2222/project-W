cmake_minimum_required(VERSION 3.25)

project(audio-player)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Disable static SDL" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


option(USE_WAYLAND "Build with Wayland support" OFF)
option(USE_X11 "Build with X11 support" OFF)


find_package(SDL3 3.4 CONFIG QUIET)
if (NOT TARGET SDL3::SDL3)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.14.tar.gz
        URL_HASH SHA256=9D57B178FB297E121EF2605275937B7AFAA7CD24D99CE1F95953E69E7A2535D6
    )
    FetchContent_MakeAvailable(SDL3)
endif()

find_package(Catch2 3 CONFIG QUIET)
if (NOT TARGET Catch2::Catch2WithMain)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.16.1
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()


find_package(Threads REQUIRED)
find_package(Dawn CONFIG QUIET)
if (NOT TARGET dawn::webgpu_dawn)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "" FORCE)
    set(DAWN_BUILD_MONOLITHIC_LIBRARY STATIC CACHE STRING "" FORCE)
    set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(DAWN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(DAWN_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(DAWN_USE_GLFW OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_CMD_TOOLS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        dawn
        URL https://github.com/google/dawn/archive/2193f6584b6d08018cfd01fb8b7a893532003559.tar.gz
        URL_HASH SHA256=8659c7cfdbfb216270942f3575fd4c38f4093433945baa4f2a87050eed0c07b0
    )
    FetchContent_MakeAvailable(dawn)
endif()


add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE dawn::webgpu_dawn)

add_subdirectory(vendor/c2-core-lib)

add_subdirectory(vendor/sdl3webgpu)
target_compile_definitions(sdl3webgpu PRIVATE WEBGPU_BACKEND_DAWN)

add_subdirectory(vendor/imgui)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

add_subdirectory(vendor/taglib)

add_subdirectory(vendor/miniaudio)

set(CORE_SOURCES 
    src/webgpu_context.cpp
    src/window.cpp
    src/webgpu_utils.cpp
    src/render.cpp
    src/audio.cpp
    src/music_player_ui.cpp
    src/track_artwork.cpp
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


# Also support SDKs that provide shared libraries.
if(WIN32)
    # Dawn loads the D3D12 shader compiler dynamically, even for a clear-only pass.
    FetchContent_Declare(dxc
        URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/dxc_2026_09_29.zip
        URL_HASH SHA256=ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1
    )
    FetchContent_MakeAvailable(dxc)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(dxc_arch x86)
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64" OR
           CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(dxc_arch arm64)
    else()
        set(dxc_arch x64)
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            "${dxc_SOURCE_DIR}/bin/${dxc_arch}/dxcompiler.dll"
            "${dxc_SOURCE_DIR}/bin/${dxc_arch}/dxil.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/dxc"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${dxc_SOURCE_DIR}/LICENCE-MIT.txt"
            "${dxc_SOURCE_DIR}/LICENSE-LLVM.txt"
            "${dxc_SOURCE_DIR}/LICENSE-MS.txt"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/licenses/dxc"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endif()

include(CTest)
include(Catch)

add_executable(audio-tests tests/test_player.cpp tests/dsp_test.cpp)
target_link_libraries(audio-tests PRIVATE audio-core Catch2::Catch2WithMain)


if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET audio-tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:audio-tests>
        VERBATIM
    )
endif()
catch_discover_tests(audio-tests)
