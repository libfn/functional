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

set(TESTS_PFN_SOURCES
    pfn/expected.cpp
)

include(TargetGenerator)

# Generate separate target for each individual test source
foreach(source IN ITEMS ${TESTS_FN_SOURCES} ${TESTS_PFN_SOURCES})
    string(REGEX REPLACE "^(fn|pfn)/(detail|)/?([^\.]+)\.cpp$" "\\1_\\2\\3" root_name ${source})
    string(REGEX REPLACE "^(fn|pfn)/.*$" "\\1" subdir ${source})
    setup_target_for_file(
        NAME "tests_${root_name}"
        SOURCE "${source}"
        DEPENDENCIES "include_${subdir}" tests_util Catch2::Catch2WithMain
        COMPILE_OPTIONS -Wall -Wextra -Wpedantic "$<$<CONFIG:DEBUG>:-O0>"
        TEST_OPTIONS -r console
        TEST_LABELS "tests_${subdir}" ${root_name}
        ADD_TEST
    )
    unset(root_name)
endforeach()

# Generate C++20 targets for pfn tests
foreach(source IN ITEMS ${TESTS_PFN_SOURCES})
    string(REGEX REPLACE "^(pfn)/(detail|)/?([^\.]+)\.cpp$" "\\1_\\2\\3" root_name ${source})
    setup_target_for_file(
        NAME "tests_${root_name}_cxx20"
        SOURCE "${source}"
        DEPENDENCIES include_pfn Catch2::Catch2WithMain
        COMPILE_OPTIONS -Wall -Wextra -Wpedantic "$<$<CONFIG:DEBUG>:-O0>"
        TEST_OPTIONS -r console
        TEST_LABELS tests_pfn cxx20 ${root_name}
        ADD_TEST
    )
    set_property(TARGET "tests_${root_name}_cxx20" PROPERTY CXX_STANDARD 20)
    unset(root_name)
endforeach()

# TODO change examples into subproject and switch to add_subdirectory
set(TESTS_EXAMPLES_SOURCES
    examples/simple.cpp
)

add_executable(tests_examples ${TESTS_EXAMPLES_SOURCES})
target_link_libraries(tests_examples include_fn Catch2::Catch2WithMain)
target_compile_options(tests_examples PRIVATE -Wall -Wextra -Wpedantic)

add_test(
    NAME tests_examples
    COMMAND tests_examples -r console
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_property(TEST tests_examples PROPERTY LABELS tests_examples)
