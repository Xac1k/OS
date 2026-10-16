cmake_minimum_required(VERSION 3.18)
project(thumbgen)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED)

add_executable(thumbgen main.cpp)
target_link_libraries(thumbgen PRIVATE ${OpenCV_LIBS} ${Boost_LIBRARIES})

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

if(SOURCES)
    add_library(ThumbnailSources STATIC ${SOURCES})
    target_include_directories(ThumbnailSources PUBLIC ${CMAKE_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
    target_link_libraries(thumbgen PRIVATE ThumbnailSources)
else()
    message(STATUS "[SimilarImageFinder] src/ is empty, building main.cpp only")
endif()