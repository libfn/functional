cmake_minimum_required(VERSION 3.25)
project(examples)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(TESTS_EXAMPLES_SOURCES
    simple/main.cpp
)

# TODO add 20 when we are compatible with C++20
foreach(mode 23)
    foreach(source IN ITEMS ${TESTS_EXAMPLES_SOURCES})
        string(REGEX REPLACE "^([^\.]+)/main\.cpp$" "\\1" root_name ${source})
        set(target "tests_examples_${root_name}_cxx${mode}")

        add_executable("${target}" ${TESTS_EXAMPLES_SOURCES})
        target_link_libraries("${target}" include_fn Catch2::Catch2WithMain)
        append_compilation_options("${target}" WARNINGS)
        add_dependencies("cxx${mode}" "${target}")
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")
        target_compile_definitions("${target}" PRIVATE LIBFN_MODE=${mode})

        add_test(
            NAME "${target}"
            COMMAND "${target}" -r console
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_property(TEST "${target}" PROPERTY LABELS tests_examples "cxx${mode}")

        unset(target)
    endforeach()
endforeach()
