cmake_minimum_required(VERSION 3.18)
project(UserThreads)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS context)

add_executable(UserThreads main.cpp)
target_link_libraries(UserThreads PRIVATE Boost::context)

target_include_directories(UserThreads PUBLIC ${Boost_INCLUDE_DIRS})
