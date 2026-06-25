cmake_minimum_required(VERSION 3.25)
project(tests)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# tests_util: shared test helpers. Links include_pfn so static_check.hpp's fn
# includes resolve at cxx_std_20 (include_pfn exposes include/, containing fn/).
# Pls keep the filenames sorted
set(TESTS_UTIL_HEADERS
    util/helper_types.hpp
    util/static_check.hpp
)

add_library(tests_util INTERFACE)
target_link_libraries(tests_util INTERFACE include_pfn)
target_sources(tests_util INTERFACE
    FILE_SET tests_util_headers
    TYPE HEADERS
    FILES ${TESTS_UTIL_HEADERS})
target_include_directories(tests_util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Generate sentinel target for each individual header, as a basic sanity check
foreach(mode 20 23)
    if(${mode} EQUAL 23 AND DISABLE_CXX23)
        message(STATUS "C++23 support is disabled, skipping tests_util sentinel targets for C++23")
        continue()
    endif()

    foreach(source IN ITEMS ${TESTS_UTIL_HEADERS})
        string(REGEX REPLACE "^(util)/([^\.]+)\.hpp$" "\\1_\\2" root_name ${source})
        set(target "sentinel_${root_name}_cxx${mode}")

        create_target_for_file(
            NAME "${target}"
            SOURCE "${source}"
            NEW_SOURCE "#include <${source}>\nint main() {}\n"
            SOURCE_ROOT "${CMAKE_BINARY_DIR}/sentinel"
            DEPENDENCIES tests_util
        )
        append_compilation_options("${target}" WARNINGS)
        add_dependencies("cxx${mode}" "${target}")
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")
        target_compile_definitions("${target}" PRIVATE LIBFN_MODE=${mode})

        unset(target)
        unset(root_name)
    endforeach()
endforeach()

### tests/pfn

set(TESTS_PFN_SOURCES
    pfn/expected.cpp
    pfn/expected_validation.cpp
    pfn/functional.cpp
    pfn/utility.cpp
)

include(TargetGenerator)
include(CompilationOptions)

# Generate separate target for each individual test source
foreach(mode 20 23)
    if(${mode} EQUAL 23 AND DISABLE_CXX23)
        message(STATUS "C++23 support is disabled, skipping tests_pfn targets for C++23")
        continue()
    endif()

    foreach(source IN ITEMS ${TESTS_PFN_SOURCES})
        string(REGEX REPLACE "^(pfn)/(detail|)/?([^\.]+)\.cpp$" "\\1_\\2\\3" root_name ${source})
        set(target "tests_${root_name}_cxx${mode}")

        create_target_for_file(
            NAME "${target}"
            SOURCE "${source}"
            DEPENDENCIES include_pfn tests_util Catch2::Catch2WithMain
        )
        append_compilation_options("${target}" WARNINGS OPTIMIZATION TESTS)
        add_dependencies("cxx${mode}" "${target}")
        add_dependencies("tests" "${target}")
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")
        target_compile_definitions("${target}" PRIVATE LIBFN_MODE=${mode})

        add_test(
            NAME "${target}"
            COMMAND "${target}" -r console
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_property(TEST "${target}" PROPERTY LABELS tests_pfn "cxx${mode}" "${root_name}")

        unset(target)
        unset(root_name)
    endforeach()
endforeach()

### tests/fn

# Pls keep the filenames sorted
set(TESTS_FN_SOURCES
    fn/detail/functional.cpp
    fn/detail/fwd_macro.cpp
    fn/detail/macro_deduced_return.cpp
    fn/detail/meta.cpp
    fn/detail/pack_impl.cpp
    fn/detail/traits.cpp
    fn/detail/variadic_union.cpp
    fn/and_then.cpp
    fn/choice.cpp
    fn/concepts.cpp
    fn/discard.cpp
    fn/expected.cpp
    fn/expected_polyfill.cpp
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

set(TESTS_FN_SOURCES_CXX20
    fn/detail/functional.cpp
    fn/detail/fwd_macro.cpp
    fn/detail/macro_deduced_return.cpp
    fn/detail/meta.cpp
    fn/detail/pack_impl.cpp
    fn/detail/traits.cpp
    fn/detail/variadic_union.cpp
    fn/choice.cpp
    fn/concepts.cpp
    fn/expected.cpp
    fn/functional.cpp
    fn/functor.cpp
    fn/optional.cpp
    fn/pack.cpp
    fn/sum.cpp
    fn/utility.cpp
)

# Generate separate target for each individual test source
foreach(mode 20 23)
    if(${mode} EQUAL 23 AND DISABLE_CXX23)
        message(STATUS "C++23 support is disabled, skipping tests_fn targets for C++23")
        continue()
    endif()

    foreach(source IN ITEMS ${TESTS_FN_SOURCES})
        # Some fn files are not compatible with C++20 - see the list above.
        if(mode EQUAL 20 AND NOT source IN_LIST TESTS_FN_SOURCES_CXX20)
            continue()
        endif()

        # fn is not yet MSVC-compatible: meta.hpp derives type ordering from
        # GCC/Clang-specific __PRETTY_FUNCTION__. MSVC builds only pfn for now.
        if(MSVC)
            continue()
        endif()

        string(REGEX REPLACE "^(fn)/(detail|)/?([^\.]+)\.cpp$" "\\1_\\2\\3" root_name ${source})
        set(target "tests_${root_name}_cxx${mode}")

        create_target_for_file(
            NAME "${target}"
            SOURCE "${source}"
            DEPENDENCIES tests_util Catch2::Catch2WithMain
        )
        append_compilation_options("${target}" WARNINGS OPTIMIZATION TESTS)
        add_dependencies("cxx${mode}" "${target}")
        add_dependencies("tests" "${target}")
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")
        target_compile_definitions("${target}" PRIVATE LIBFN_MODE=${mode})

        add_test(
            NAME "${target}"
            COMMAND "${target}" -r console
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_property(TEST "${target}" PROPERTY LABELS tests_fn "cxx${mode}" "${root_name}")

        unset(target)
        unset(root_name)
    endforeach()
endforeach()
