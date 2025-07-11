add_library(pv_orca_napi SHARED pv_orca_napi.c)
target_compile_options(pv_orca_napi PRIVATE ${W_ERROR_COMPILE_OPTIONS})

set_target_properties(
        pv_orca_napi
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME "pv_orca"
        SUFFIX ".node"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
)

set(NODE_DEBUG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/Debug${NODE_DUMP})
set(NODE_RELEASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/Release${NODE_DUMP})

file(MAKE_DIRECTORY ${NODE_DEBUG_DIR})
file(MAKE_DIRECTORY ${NODE_RELEASE_DIR})

add_custom_command(
        TARGET pv_orca_napi
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pv_orca_napi>
        $<IF:$<CONFIG:RELEASE>,${NODE_RELEASE_DIR},${NODE_DEBUG_DIR}>
)

add_dependencies(pv_orca_napi node_headers)
target_include_directories(pv_orca_napi PRIVATE ${NODE_INCLUDE_DIR})

if (${PV_TARGET_PLATFORM} STREQUAL "windows")
    add_dependencies(pv_orca_napi node_win_lib)
    target_sources(pv_orca_napi PRIVATE ${zoo-dev_SOURCE_DIR}/node/shared/delayhook.c)
endif ()
target_link_libraries(pv_orca_napi ${NODE_LIB} pv_core_runtime_static pv_orca_static)

if (${PV_TARGET_PLATFORM} STREQUAL "mac")
    target_link_options(pv_orca_napi PRIVATE "-undefined" "dynamic_lookup")
endif ()
