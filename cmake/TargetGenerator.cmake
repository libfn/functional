# Generate target for a given source file, and optionally add it to tests

function(setup_target_for_file)
    set(options ADD_TEST)
    set(oneValueArgs NAME SOURCE SOURCE_ROOT NEW_SOURCE)
    set(multiValueArgs DEPENDENCIES COMPILE_OPTIONS TEST_OPTIONS TEST_LABELS)
    cmake_parse_arguments(Generator "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED Generator_SOURCE)
        message(FATAL_ERROR "SOURCE must be set")
    endif()

    if(DEFINED Generator_TEST_OPTIONS AND (NOT Generator_ADD_TEST))
        message(FATAL_ERROR "ADD_TEST must be set if TEST_OPTIONS is set")
    endif()

    if(DEFINED Generator_TEST_LABELS AND (NOT Generator_ADD_TEST))
        message(FATAL_ERROR "ADD_TEST must be set if TEST_LABELS is set")
    endif()

    if(NOT DEFINED Generator_NAME)
        string(REGEX REPLACE "[\\./]" "-" Generator_NAME ${Generator_SOURCE})
    endif()

    if(NOT DEFINED Generator_SOURCE_ROOT)
        set(Generator_SOURCE_ROOT "${CMAKE_BINARY_DIR}/generated")
    endif()

    if(DEFINED Generator_NEW_SOURCE)
        string(REGEX REPLACE "^(.*)\\.[a-zA-Z]+$" "${Generator_SOURCE_ROOT}/\\1_Source.cpp" file_name ${Generator_SOURCE})
        file(WRITE ${file_name} "${Generator_NEW_SOURCE}")
        add_executable(${Generator_NAME} ${file_name} ${Generator_SOURCE})
        unset(file_name)
    else()
        add_executable(${Generator_NAME} ${Generator_SOURCE})
    endif()

    target_link_libraries(${Generator_NAME} ${Generator_DEPENDENCIES})

    target_compile_options(${Generator_NAME} PRIVATE ${Generator_COMPILE_OPTIONS})

    if(Generator_ADD_TEST)
        add_test(
            NAME ${Generator_NAME}
            COMMAND ${Generator_NAME} ${Generator_TEST_OPTIONS}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        if(Generator_TEST_LABELS)
            set_property(TEST ${Generator_NAME} PROPERTY LABELS "${Generator_TEST_LABELS}")
        endif()
    endif()
endfunction()
