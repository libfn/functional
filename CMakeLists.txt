cmake_minimum_required(VERSION 3.25)
project(functional VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(Ccache)

option(COVERARE "Enable coverage report." OFF)
if(COVERAGE)
  include(CodeCoverage)
  include(Coverage)
endif()

include(Catch2)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(
  -march=native -Wall -Wextra -Wpedantic "$<$<CONFIG:DEBUG>:-O0>"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Workaround for std::expected not available in clang
  add_compile_options(
    -stdlib=libstdc++ -D__cpp_concepts=202002 -Wno-builtin-macro-redefined -Wno-missing-braces
  )
endif()

add_subdirectory(include)

enable_testing()
add_subdirectory(tests)
