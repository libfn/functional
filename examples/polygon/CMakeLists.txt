cmake_minimum_required(VERSION 3.25)
project(examples_polygon)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(TESTS_EXAMPLES_POLYGON_SOURCES
    main.cpp
)

# TODO add 20 when we are compatible with C++20
foreach(mode 23)
    set(target "examples_polygon_cxx${mode}")

    add_executable("${target}" ${TESTS_EXAMPLES_POLYGON_SOURCES})
    target_link_libraries("${target}" include_fn)
    append_compilation_options("${target}" WARNINGS)
    add_dependencies("cxx${mode}" "${target}")
    set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")
    target_compile_definitions("${target}" PRIVATE LIBFN_MODE=${mode})

    unset(target)
endforeach()
