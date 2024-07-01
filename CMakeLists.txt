cmake_minimum_required(VERSION 3.25)
project(functional VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(Ccache)

option(USE_NIX "Use nix packages to resolve dependencies." OFF)

option(COVERARE "Enable coverage report." OFF)
option(DOCS     "Generate documentation." OFF)

if(COVERAGE)
  include(CodeCoverage)
  include(Coverage)
endif()

if (DOCS)
  include(Docs)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(
  -march=native -Wall -Wextra -Wpedantic "$<$<CONFIG:DEBUG>:-O0>"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -stdlib=libc++ -Wno-missing-braces -Wno-unused-command-line-argument
  )
  add_link_options(
    -lc++
  )
endif()

add_subdirectory(include)

include(Catch2)
enable_testing()
add_subdirectory(tests)

install(FILES README.md LICENSE.md
    DESTINATION share/doc/functional)
