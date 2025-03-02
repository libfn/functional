# Add compilation options appropriate for the current compiler

# Note, we do not select libc++ like an example below. Instead the user should
# use the CXXFLAGS environment variable for this option.
#
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND (NOT APPLE))
#     add_compile_options(-stdlib=libc++)
#     add_link_options(-lc++)
# endif()


function(append_compilation_options)
    set(options WARNINGS OPTIMIZATION INTERFACE)
    set(Options_NAME ${ARGV0})
    cmake_parse_arguments(Options "${options}" "" "" ${ARGN})

    if(NOT DEFINED Options_NAME)
        message(FATAL_ERROR "NAME must be set")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        if(Options_WARNINGS)
            target_compile_options(${Options_NAME} PRIVATE /W4)
        endif()

        if(Options_OPTIMIZATION)
            target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,/Od,/Ox>)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?Clang")
        if(Options_WARNINGS)
            target_compile_options(${Options_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-redundant-move)
        endif()

        if(Options_OPTIMIZATION)
            target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,-O0,-O2>)
        endif()

        if(Options_INTERFACE)
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                target_compile_options(${Options_NAME} INTERFACE -Wno-non-template-friend)
            endif()

            target_compile_options(${Options_NAME} INTERFACE -Wno-missing-braces)
        endif()
    endif()
endfunction()
