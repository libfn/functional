cmake_minimum_required(VERSION 3.25)
project(tests)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(TESTS_UTIL_SOURCES
    util/static_check.hpp
)
add_library(tests_util INTERFACE ${TESTS_UTIL_SOURCES})
target_include_directories(tests_util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests_util INTERFACE include_fn)


# Pls keep the filenames sorted
set(TESTS_FN_SOURCES
    fn/detail/fwd_macro.cpp
    fn/detail/meta.cpp
    fn/detail/variadic_union.cpp
    fn/and_then.cpp
    fn/choice.cpp
    fn/concepts.cpp
    fn/expected.cpp
    fn/fail.cpp
    fn/filter.cpp
    fn/functional.cpp
    fn/functor.cpp
    fn/inspect_error.cpp
    fn/inspect.cpp
    fn/optional.cpp
    fn/or_else.cpp
    fn/pack.cpp
    fn/recover.cpp
    fn/sum.cpp
    fn/transform_error.cpp
    fn/transform.cpp
    fn/utility.cpp
    fn/value_or.cpp
)

include(TargetGenerator)

# Generate separate target for each individual test source
foreach(source IN ITEMS ${TESTS_FN_SOURCES})
    string(REGEX REPLACE "^fn/(detail|)/?([^\.]+)\.cpp$" "\\1\\2" root_name ${source})
    setup_target_for_file(
        NAME "tests_fn_${root_name}"
        SOURCE "${source}"
        DEPENDENCIES include_fn tests_util Catch2::Catch2WithMain
        COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wno-missing-braces "$<$<CONFIG:DEBUG>:-O0>"
        TEST_OPTIONS -r console
        TEST_LABELS tests_fn ${root_name}
        ADD_TEST
    )
    unset(root_name)
endforeach()

# Pls keep the filenames sorted
set(TESTS_EXAMPLES_SOURCES
    examples/simple.cpp
)

foreach(source IN ITEMS ${TESTS_EXAMPLES_SOURCES})
    string(REGEX REPLACE "^examples/([^\.]+)\.cpp$" "\\1" root_name ${source})
    setup_target_for_file(
        NAME "tests_examples_${root_name}"
        SOURCE "${source}"
        DEPENDENCIES include_fn Catch2::Catch2WithMain
        COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wno-missing-braces
        TEST_OPTIONS -r console
        TEST_LABELS tests_examples ${root_name}
        ADD_TEST
    )
    unset(root_name)
endforeach()
