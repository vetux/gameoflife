cmake_minimum_required(VERSION 3.0.0)

project(GameOfLife)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE -O3)
endif ()

add_compile_definitions(XENGINE_EXPORT=)

link_directories(lib/bin/)
include_directories(lib/include/)

file(GLOB_RECURSE SRC src/*.cpp src/*.c)

add_executable(gameoflife ${SRC})

target_include_directories(gameoflife PRIVATE src/)
target_link_libraries(gameoflife xengine)

file(GLOB_RECURSE ASSETS asset/*)
file(COPY ${ASSETS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/asset)

