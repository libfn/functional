cmake_minimum_required(VERSION 3.25)
project(functional VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(DISABLE_FETCH_CONTENT "Do not use FetchContent for dependencies." OFF)
option(DISABLE_CCACHE_DETECTION "Do not use ccache as CMAKE_CXX_COMPILER_LAUNCHER." OFF)
option(COVERAGE "Enable coverage report." OFF)
option(DOCS     "Generate documentation." OFF)
option(TESTS    "Enable unit tests and examples." ON)

if(DOCS AND (NOT TESTS))
    message(FATAL_ERROR "TESTS must be set if DOCS is set")
endif()

if(COVERAGE AND (NOT TESTS))
    message(FATAL_ERROR "TESTS must be set if COVERAGE is set")
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
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -stdlib=libc++
  )
  add_link_options(
    -lc++
  )
endif()

add_subdirectory(include)

if(TESTS)
    include(Catch2)
    enable_testing()
    add_subdirectory(tests)
endif()

install(FILES README.md LICENSE.md
    DESTINATION share/doc/functional)
