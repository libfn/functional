# Generate target for a given source file, and optionally add it to tests

function(create_target_for_file)
    set(oneValueArgs NAME SOURCE SOURCE_ROOT NEW_SOURCE)
    set(multiValueArgs DEPENDENCIES COMPILE_OPTIONS)
    cmake_parse_arguments(Generator "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED Generator_SOURCE)
        message(FATAL_ERROR "SOURCE must be set")
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
endfunction()
