cmake_minimum_required(VERSION 3.13)
project(orca_demo)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Release)

include_directories("${PROJECT_SOURCE_DIR}/../../include")

add_executable(orca_demo orca_demo.c)

if (NOT WIN32)
    target_link_libraries(orca_demo dl)
endif()
