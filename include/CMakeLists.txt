cmake_minimum_required(VERSION 3.25)
project(include)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(INCLUDE_SOURCE_FILES
    functional/detail/closure.hpp
    functional/detail/concepts.hpp
    functional/detail/fwd_macro.hpp
    functional/detail/traits.hpp
    functional/and_then.hpp
    functional/concepts.hpp
    functional/fail.hpp
    functional/filter.hpp
    functional/functor.hpp
    functional/fwd.hpp
    functional/inspect_error.hpp
    functional/inspect.hpp
    functional/or_else.hpp
    functional/recover.hpp
    functional/transform_error.hpp
    functional/transform.hpp
    functional/utility.hpp
)

add_library(${PROJECT_NAME} INTERFACE ${INCLUDE_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
