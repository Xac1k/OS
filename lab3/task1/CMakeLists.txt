cmake_minimum_required(VERSION 3.18)
project(SimilarImageFinder)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

add_executable(SimilarImageFinder main.cpp)
target_link_libraries(SimilarImageFinder PRIVATE ${OpenCV_LIBS})

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

if(SOURCES)
    add_library(EventServiceSources STATIC ${SOURCES})
    target_include_directories(EventServiceSources PUBLIC ${CMAKE_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(SimilarImageFinder PRIVATE EventServiceSources)
else()
    message(STATUS "[SimilarImageFinder] src/ is empty, building main.cpp only")
endif()