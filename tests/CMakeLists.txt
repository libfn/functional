cmake_minimum_required(VERSION 3.25)
project(tests)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(TESTS_UTIL_SOURCES
    util/static_check.hpp
)
add_library(tests_util INTERFACE ${TESTS_UTIL_SOURCES})
set_property(TARGET include_fn PROPERTY CXX_STANDARD 23)
target_include_directories(tests_util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests_util INTERFACE include_fn)

### tests/pfn

set(TESTS_PFN_SOURCES
    pfn/expected.cpp
)

include(TargetGenerator)

# Generate separate target for each individual test source
foreach(cxxrel 20 23)
    foreach(source IN ITEMS ${TESTS_PFN_SOURCES})
        string(REGEX REPLACE "^(pfn)/(detail|)/?([^\.]+)\.cpp$" "\\1_\\2\\3" root_name ${source})
        set(target "tests_${root_name}_cxx${cxxrel}")

        create_target_for_file(
            NAME "${target}"
            SOURCE "${source}"
            DEPENDENCIES include_pfn Catch2::Catch2WithMain
            COMPILE_OPTIONS -Wall -Wextra -Wpedantic "$<$<CONFIG:DEBUG>:-O0>"
        )
        add_dependencies("cxx${cxxrel}" "${target}")
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${cxxrel}")

        add_test(
            NAME "${target}"
            COMMAND "${target}" -r console
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_property(TEST "${target}" PROPERTY LABELS tests_pfn "cxx${cxxrel}" "${root_name}")

        unset(target)
        unset(root_name)
    endforeach()
endforeach()

### tests/fn

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

# Generate separate target for each individual test source
# TODO add 20 when we are compatible with C++20
foreach(cxxrel 23)
    foreach(source IN ITEMS ${TESTS_FN_SOURCES})
        string(REGEX REPLACE "^(fn)/(detail|)/?([^\.]+)\.cpp$" "\\1_\\2\\3" root_name ${source})
        set(target "tests_${root_name}_cxx${cxxrel}")

        create_target_for_file(
            NAME "${target}"
            SOURCE "${source}"
            DEPENDENCIES include_fn tests_util Catch2::Catch2WithMain
            COMPILE_OPTIONS -Wall -Wextra -Wpedantic "$<$<CONFIG:DEBUG>:-O0>"
        )
        add_dependencies("cxx${cxxrel}" "${target}")
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${cxxrel}")

        add_test(
            NAME "${target}"
            COMMAND "${target}" -r console
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_property(TEST "${target}" PROPERTY LABELS tests_fn "cxx${cxxrel}" "${root_name}")

        unset(target)
        unset(root_name)
    endforeach()
endforeach()

# TODO change examples into subproject and switch to add_subdirectory
set(TESTS_EXAMPLES_SOURCES
    examples/simple.cpp
)

# TODO add 20 when we are compatible with C++20
foreach(cxxrel 23)
    set(target "tests_examples_cxx${cxxrel}")

    add_executable("${target}" ${TESTS_EXAMPLES_SOURCES})
    target_link_libraries("${target}" include_fn Catch2::Catch2WithMain)
    target_compile_options("${target}" PRIVATE -Wall -Wextra -Wpedantic)
    add_dependencies("cxx${cxxrel}" "${target}")
    set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${cxxrel}")

    add_test(
        NAME "${target}"
        COMMAND "${target}" -r console
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_property(TEST "${target}" PROPERTY LABELS tests_examples "cxx${cxxrel}")

    unset(target)
endforeach()
