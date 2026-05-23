cmake_minimum_required(VERSION 3.25)

# CMAKE_EXPORT_COMPILE_COMMANDS must be set before project() to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for clangd")

project(functional VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(DISABLE_FETCH_CONTENT "Do not use FetchContent for dependencies." OFF)
option(DISABLE_CCACHE_DETECTION "Do not use ccache as CMAKE_CXX_COMPILER_LAUNCHER." OFF)
option(COVERAGE "Enable coverage report." OFF)
option(DOCS     "Generate documentation." OFF)
option(TESTS    "Enable unit tests and examples." ON)
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_sanitizers_default ON)
else()
    set(_sanitizers_default OFF)
endif()
option(SANITIZERS "Enable sanitizers (Debug builds only)." ${_sanitizers_default})
unset(_sanitizers_default)

if(DOCS AND (NOT TESTS))
    message(FATAL_ERROR "DOCS=ON requires TESTS=ON")
endif()

if(COVERAGE AND (NOT TESTS))
    message(FATAL_ERROR "COVERAGE=ON requires TESTS=ON")
endif()

if(SANITIZERS AND CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "SANITIZERS=ON requires CMAKE_BUILD_TYPE=Debug (got '${CMAKE_BUILD_TYPE}')"|)
endif()

if(COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|(Apple)?Clang)")
        include(CodeCoverage)
        include(Coverage)
    else()
        message(FATAL_ERROR "COVERAGE not supported by compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if (DOCS)
    include(Docs)
endif()

include(Ccache)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Phony targets for all supported C++ versions
add_custom_target(cxx20)
add_custom_target(cxx23)

add_subdirectory(include)

if(TESTS)
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
