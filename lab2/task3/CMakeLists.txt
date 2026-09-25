cmake_minimum_required(VERSION 3.18)

project(MiniShell)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(MiniShell main.cpp)

FILE(GLOB_RECURSE RESOURCES "utils/*.cpp" "utils/*.h")
add_library(utils ${RESOURCES})

target_link_libraries(MiniShell PRIVATE utils)