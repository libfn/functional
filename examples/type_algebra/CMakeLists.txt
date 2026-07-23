cmake_minimum_required(VERSION 3.25)
project(examples_type_algebra)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(EXAMPLES_TYPE_ALGEBRA_SOURCES
    main.cpp
)

foreach(mode 20 23 26)
    if (NOT VALIDATE_CXX23 AND mode EQUAL 23)
        continue()
    endif()

    if (NOT VALIDATE_CXX26 AND mode EQUAL 26)
        continue()
    endif()

    # Current releases of MSVC only support C++20
    if(MSVC AND NOT (mode EQUAL 20))
        continue()
    endif()

    if(mode EQUAL 26)
        set(entry_point include_fn_cxx26)
    else()
        set(entry_point include_fn)
    endif()

    set(target "examples_type_algebra_cxx${mode}")

    add_executable("${target}" ${EXAMPLES_TYPE_ALGEBRA_SOURCES})
    target_link_libraries("${target}" "${entry_point}")
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
