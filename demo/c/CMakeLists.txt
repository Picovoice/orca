cmake_minimum_required(VERSION 3.13)
project(orca_demo_c)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Release)

set(COMMON_LIBS dl)
include_directories("${PROJECT_SOURCE_DIR}/../../include")

add_executable(orca_demo orca_demo.c)

add_executable(orca_demo_streaming orca_demo_streaming.c)
target_include_directories(orca_demo_streaming PRIVATE dr_libs)

if (NOT WIN32)
    target_link_libraries(orca_demo ${COMMON_LIBS})
    target_link_libraries(orca_demo_streaming ${COMMON_LIBS})
endif()