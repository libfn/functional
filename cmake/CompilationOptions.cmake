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
            # disable C4456: declaration of 'b' hides previous local declaration
            target_compile_options(${Options_NAME} PRIVATE /W4 /wd4456)
        endif()

        if(Options_OPTIMIZATION)
            target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,/Od,/Ox>)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?Clang")
        if(Options_WARNINGS)
            target_compile_options(${Options_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-redundant-move)
        endif()

        if(Options_OPTIMIZATION)
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
                target_compile_options(${Options_NAME} PRIVATE -O0 -fsanitize=address -static-libasan -fno-omit-frame-pointer)
                target_link_options(${Options_NAME} PRIVATE -fsanitize=address)
            else()
                target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,-O0 -fno-omit-frame-pointer,-O2>)
            endif()
        endif()

        if(Options_INTERFACE)
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                target_compile_options(${Options_NAME} INTERFACE -Wno-non-template-friend)
            endif()

            target_compile_options(${Options_NAME} INTERFACE -Wno-missing-braces)
        endif()
    endif()
endfunction()
