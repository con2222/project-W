cmake_minimum_required(VERSION 3.20)
project(miniaudio LANGUAGES C)

set(CORE_SOURCES
    miniaudio.h
    miniaudio.c
)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(${PROJECT_NAME} PUBLIC c_std_99)