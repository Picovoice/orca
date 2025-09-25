macro(fetch_peer_module MODULE_NAME SELECTED_COMMIT)
    if (DEFINED PV_PEER_MODULE_DIR AND NOT EXISTS "${PV_PEER_MODULE_DIR}")
        file(MAKE_DIRECTORY "${PV_PEER_MODULE_DIR}")
    endif ()
    if (DEFINED CI_JOB_TOKEN)
        set(MODULE_REMOTE "https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/picovoice/${MODULE_NAME}.git")
    else ()
        set(MODULE_REMOTE "git@gitlab.com:picovoice/${MODULE_NAME}.git")
    endif ()

    set(EXPECTED_COMMIT ${SELECTED_COMMIT})
    string(REPLACE "-" "_" cleaned_module_name ${MODULE_NAME})
    if (DEFINED ENV{PV_CICD_${cleaned_module_name}_COMMIT_HASH})
        set(EXPECTED_COMMIT $ENV{PV_CICD_${cleaned_module_name}_COMMIT_HASH})
        message(WARNING "Overriding commit `${SELECTED_COMMIT}` for `${MODULE_NAME}` with commit `${EXPECTED_COMMIT}`")
    endif ()

    get_filename_component(MODULE_DIR "${PV_PEER_MODULE_DIR}/${MODULE_NAME}" ABSOLUTE)
    if (NOT EXISTS "${MODULE_DIR}")
        message(STATUS "Cloning ${MODULE_NAME} to ${MODULE_DIR}")
        execute_process(
          COMMAND git clone --depth 100 --no-single-branch --shallow-submodules --recurse-submodules ${MODULE_REMOTE} ${MODULE_DIR}
          RESULT_VARIABLE res
        )
        if (NOT res EQUAL 0)
            message(FATAL_ERROR "Failed to clone ${MODULE_NAME}")
        endif ()
    else ()
        if (DEFINED CI_JOB_TOKEN)
            # Remote needs to change to match current GitLab job
            message(STATUS "Setting remote to origin ${MODULE_REMOTE}")
            execute_process(
              COMMAND git remote set-url origin "${MODULE_REMOTE}"
              WORKING_DIRECTORY "${MODULE_DIR}"
              RESULT_VARIABLE res
            )
            if (NOT res EQUAL 0)
                message(FATAL_ERROR "Failed to set remote for ${MODULE_NAME} to ${MODULE_REMOTE}")
            endif ()

            message(STATUS "Syncing submodules with new remote")
            execute_process(
              COMMAND git submodule sync --recursive
              WORKING_DIRECTORY "${MODULE_DIR}"
              RESULT_VARIABLE res
            )
            if (NOT res EQUAL 0)
                message(FATAL_ERROR "Failed to sync submodules")
            endif ()
        endif ()
    endif()

    execute_process(
      COMMAND git rev-parse HEAD
      WORKING_DIRECTORY ${MODULE_DIR}
      OUTPUT_VARIABLE CURRENT_COMMIT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (NOT "${CURRENT_COMMIT}" STREQUAL "${EXPECTED_COMMIT}")
        if (SETUP_REPO)
            message(STATUS "Fetching ${EXPECTED_COMMIT} from ${MODULE_NAME}")
            execute_process(
              COMMAND git fetch origin ${EXPECTED_COMMIT}
              WORKING_DIRECTORY "${MODULE_DIR}"
              RESULT_VARIABLE res
            )
            if (NOT res EQUAL 0)
                message(FATAL_ERROR "Failed to fetch ${MODULE_NAME} at commit ${EXPECTED_COMMIT}")
            endif ()
        else ()
            message(FATAL_ERROR "❌ `${MODULE_NAME}` is at ${CURRENT_COMMIT}, but expected ${EXPECTED_COMMIT}.")
        endif ()
    endif ()

    if (SETUP_REPO)
        message(STATUS "Cleaning ${MODULE_NAME} directory")
        execute_process(
          COMMAND git clean -dfx
          WORKING_DIRECTORY "${MODULE_DIR}"
          RESULT_VARIABLE res
        )
        if (NOT res EQUAL 0)
            message(FATAL_ERROR "Failed to clean ${MODULE_NAME} directory")
        endif ()

        message(STATUS "Resetting to ${EXPECTED_COMMIT}")
        execute_process(
          COMMAND git reset --hard ${EXPECTED_COMMIT}
          WORKING_DIRECTORY "${MODULE_DIR}"
          RESULT_VARIABLE res
        )
        if (NOT res EQUAL 0)
            message(FATAL_ERROR "Failed to reset to commit ${EXPECTED_COMMIT}")
        endif ()

        message(STATUS "Initializing submodules for ${EXPECTED_COMMIT}")
        execute_process(
          COMMAND git submodule update --init --recursive --force
          WORKING_DIRECTORY "${MODULE_DIR}"
          RESULT_VARIABLE res
        )
        if (NOT res EQUAL 0)
            message(FATAL_ERROR "Failed to initialize submodules at commit ${EXPECTED_COMMIT}")
        endif ()

    else ()
        include(FetchContent)

        # FetchContent_Populate is deprecated
        # Need to update to FetchContent_MakeAvailable method once our min version is 3.30
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
            cmake_policy(SET CMP0169 OLD)
        endif ()
        FetchContent_Declare(
          ${MODULE_NAME}
          SOURCE_DIR "${MODULE_DIR}"
        )
        FetchContent_Populate(${MODULE_NAME})
    endif ()
endmacro()
