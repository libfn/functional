cmake_minimum_required(VERSION 3.25)

# CMAKE_EXPORT_COMPILE_COMMANDS must be set before project() to work; PROJECT_IS_TOP_LEVEL isn't available here.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for clangd.")
endif()

project(functional VERSION 0.0.1 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Symlink compile_commands.json as a relative path to the source root, for clangd and other tools.
if(CMAKE_EXPORT_COMPILE_COMMANDS AND PROJECT_IS_TOP_LEVEL AND NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    file(RELATIVE_PATH symlink_target "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/compile_commands.json")
    file(CREATE_LINK
        "${symlink_target}"
        "${PROJECT_SOURCE_DIR}/compile_commands.json"
        RESULT symlink_result
        SYMBOLIC
    )
    if(NOT symlink_result STREQUAL "0")
        message(WARNING "Could not create compile_commands.json symlink: ${symlink_result}")
    endif()
    unset(symlink_target)
    unset(symlink_result)
endif()

option(DISABLE_FETCH_CONTENT "Do not use FetchContent for dependencies." OFF)
option(DISABLE_CCACHE_DETECTION "Do not use ccache as CMAKE_CXX_COMPILER_LAUNCHER." OFF)

# Docs and coverage options are only relevant for top level builds.
if(PROJECT_IS_TOP_LEVEL)
    option(LIBFN_COVERAGE "Enable coverage report." OFF)
    option(LIBFN_DOCS     "Generate documentation." OFF)
elseif(LIBFN_COVERAGE OR LIBFN_DOCS)
    # e.g. accidentaly set in cache by a parent project. Names were chosen to be specific but ¯\_(ツ)_/¯
    message(FATAL_ERROR "LIBFN_COVERAGE=ON and LIBFN_DOCS=ON are only supported when functional is top level.")
endif()

# We want to allow tests even when our project is not a top level, e.g. for troubleshooting.
option(LIBFN_TESTS    "Enable unit tests and examples." ON)

# Do not enable sanitizers by default when our project is not a top level or when it is not Debug.
if(NOT PROJECT_IS_TOP_LEVEL)
    set(sanitizers_default OFF)
elseif(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(sanitizers_default ON)
else()
    set(sanitizers_default OFF)
endif()
option(LIBFN_SANITIZERS "Enable sanitizers (Debug builds only)." ${sanitizers_default})
unset(sanitizers_default)

if(LIBFN_DOCS AND (NOT LIBFN_TESTS))
    message(FATAL_ERROR "LIBFN_DOCS=ON requires LIBFN_TESTS=ON")
endif()

if(LIBFN_COVERAGE AND (NOT LIBFN_TESTS))
    message(FATAL_ERROR "LIBFN_COVERAGE=ON requires LIBFN_TESTS=ON")
endif()

if(LIBFN_SANITIZERS AND CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "LIBFN_SANITIZERS=ON requires CMAKE_BUILD_TYPE=Debug (got '${CMAKE_BUILD_TYPE}')")
endif()

if(PROJECT_IS_TOP_LEVEL)
    if(LIBFN_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|(Apple)?Clang)")
            include(CodeCoverage)
            include(Coverage)
        else()
            message(FATAL_ERROR "LIBFN_COVERAGE not supported by compiler ${CMAKE_CXX_COMPILER_ID}")
        endif()
    endif()

    if(LIBFN_DOCS)
        include(Docs)
    endif()

    include(Ccache)

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Phony targets for all supported C++ versions
add_custom_target(cxx20)
add_custom_target(cxx23)

add_subdirectory(include)

if(LIBFN_TESTS)
    # Phony targets for tests and examples
    add_custom_target(tests)
    add_custom_target(examples)

    include(Catch2)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()

install(FILES README.md LICENSE.md
    DESTINATION share/doc/functional)
