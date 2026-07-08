cmake_minimum_required(VERSION 3.25)
project(examples_readme)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(EXAMPLES_README_SOURCES
    main.cpp
)

foreach(mode 20 23)
    if (NOT VALIDATE_CXX23 AND mode EQUAL 23)
        continue()
    endif()

    # Current releases of MSVC only support C++20
    if(MSVC AND NOT (mode EQUAL 20))
        continue()
    endif()

    set(target "examples_readme_cxx${mode}")

    add_executable("${target}" ${EXAMPLES_README_SOURCES})
    target_link_libraries("${target}" include_fn)
    append_compilation_options("${target}" WARNINGS)
    add_dependencies("cxx${mode}" "${target}")
    add_dependencies("examples" "${target}")
    add_dependencies("tests" "${target}")
    set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")
    target_compile_definitions("${target}" PRIVATE LIBFN_MODE=${mode})

    add_test(
        NAME "${target}"
        COMMAND "${target}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_property(TEST "${target}" PROPERTY LABELS examples "cxx${mode}")

    unset(target)
endforeach()
