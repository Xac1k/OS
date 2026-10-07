set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(EventService)

add_executable(EventService main.cpp)

FILE(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
FILE(GLOB_RECURSE SHARED "../Shared/*.cpp" "../Shared/*.h")

list(LENGTH SOURCES len_SOURCES)
list(LENGTH SHARED len_SHARED)

if(len_SOURCES GREATER 0 OR len_SHARED GREATER 0)
    add_library(EventServiceSources ${SOURCES} ${SHARED})
    target_include_directories(EventService PRIVATE
            ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(EventService PRIVATE EventServiceSources)
else()
    message("[EventService] Сборака только main.cpp")
endif ()

