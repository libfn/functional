cmake_minimum_required(VERSION 3.25)
project(tests)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(TESTS_SOURCE_FILES
    detail/fwd_macro.cpp
    detail/meta.cpp
    detail/variadic_union.cpp
    and_then.cpp
    choice.cpp
    concepts.cpp
    expected.cpp
    fail.cpp
    filter.cpp
    functional.cpp
    functor.cpp
    inspect_error.cpp
    inspect.cpp
    optional.cpp
    or_else.cpp
    pack.cpp
    recover.cpp
    sum.cpp
    transform_error.cpp
    transform.cpp
    utility.cpp
    value_or.cpp
)

# Generate separate target for each individual test source
include(TargetGenerator)

foreach(source IN ITEMS ${TESTS_SOURCE_FILES})
    string(REGEX REPLACE "^(detail/|)([a-z_]+)\.cpp$" "\\2" root_name ${source})
    setup_target_for_file(
        SOURCE "${source}"
        DEPENDENCIES include util Catch2::Catch2WithMain
        TEST_OPTIONS -r console
        TEST_LABELS tests_fn ${root_name}
        ADD_TEST
    )
    unset(root_name)
endforeach()
