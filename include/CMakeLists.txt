cmake_minimum_required(VERSION 3.25)
project(include)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(INCLUDE_SOURCE_FILES
    functional/detail/functional.hpp
    functional/detail/fwd_macro.hpp
    functional/detail/fwd.hpp
    functional/detail/meta.hpp
    functional/detail/pack_impl.hpp
    functional/detail/traits.hpp
    functional/detail/variadic_union.hpp
    functional/and_then.hpp
    functional/choice.hpp
    functional/concepts.hpp
    functional/expected.hpp
    functional/fail.hpp
    functional/filter.hpp
    functional/functional.hpp
    functional/functor.hpp
    functional/fwd.hpp
    functional/inspect_error.hpp
    functional/inspect.hpp
    functional/optional.hpp
    functional/or_else.hpp
    functional/pack.hpp
    functional/recover.hpp
    functional/sum.hpp
    functional/transform_error.hpp
    functional/transform.hpp
    functional/utility.hpp
    functional/value_or.hpp
)

add_library(${PROJECT_NAME} INTERFACE ${INCLUDE_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
