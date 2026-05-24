# Add compilation options appropriate for the current compiler
#
# The user is expected to select libc++ via the CXXFLAGS environment variable
# rather than from this file.

function(append_compilation_options)
    if(ARGC EQUAL 0)
        message(FATAL_ERROR "target name must be provided as the first positional argument")
    endif()
    set(Options_NAME ${ARGV0})
    set(_rest ${ARGN})
    list(REMOVE_AT _rest 0)

    set(options WARNINGS OPTIMIZATION INTERFACE TESTS)
    cmake_parse_arguments(Options "${options}" "" "" ${_rest})

    if(Options_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown options: ${Options_UNPARSED_ARGUMENTS}")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        if(Options_WARNINGS)
            # disable C4456: declaration of 'b' hides previous local declaration
            # disable C4244: 'initializing': conversion from '_Ty' to '_Ty', possible loss of data
            # disable C4101: 'e': unreferenced local variable
            target_compile_options(${Options_NAME} PRIVATE /W4 /wd4456 /wd4244 /wd4101)
        endif()

        if(Options_OPTIMIZATION)
            target_compile_options(${Options_NAME} PRIVATE $<IF:$<CONFIG:Debug>,/Od,/O2>)
        endif()

        if(Options_INTERFACE)
            target_compile_options(${Options_NAME} INTERFACE /Za /permissive-)
            # This will disable `unexpected` in global namespace from eh.h
            target_compile_definitions(${Options_NAME} INTERFACE _HAS_CXX23)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(Options_WARNINGS)
            target_compile_options(${Options_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)

            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19)
                target_compile_options(${Options_NAME} PRIVATE -Wno-c2y-extensions)
            endif()
        endif()

        if(Options_OPTIMIZATION)
            target_compile_options(${Options_NAME} PRIVATE
                $<$<CONFIG:Debug>:-O0>
                $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
                $<$<NOT:$<CONFIG:Debug>>:-O2>
            )

            if(LIBFN_SANITIZERS)
                # GCC constexpr evaluator is incompatible with UBSan instrumentation in libstdc++
                if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                    set(sanitizers address)
                else()
                    set(sanitizers address,undefined)
                endif()
                # LeakSanitizer is not supported on Darwin
                if(NOT APPLE)
                    string(APPEND sanitizers ",leak")
                endif()

                # Statically link the sanitizer runtimes that match ${sanitizers} above.
                # Apple's clang ships sanitizer runtimes as dylibs only, so skip there.
                # Clang only recognises the umbrella -static-libsan; GCC has per-sanitizer flags.
                set(static_san_flags)
                if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                    if(sanitizers MATCHES "address")
                        list(APPEND static_san_flags -static-libasan)
                    endif()
                    if(sanitizers MATCHES "leak")
                        list(APPEND static_san_flags -static-liblsan)
                    endif()
                    if(sanitizers MATCHES "undefined")
                        list(APPEND static_san_flags -static-libubsan)
                    endif()
                elseif(NOT APPLE)
                    list(APPEND static_san_flags -static-libsan)
                endif()

                target_compile_options(${Options_NAME} PRIVATE
                    $<$<CONFIG:Debug>:-fsanitize=${sanitizers}>
                    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,14>>:-funreachable-traps>
                )
                target_link_options(${Options_NAME} PRIVATE
                    $<$<CONFIG:Debug>:-fsanitize=${sanitizers}>
                    "$<$<CONFIG:Debug>:${static_san_flags}>"
                )

                unset(sanitizers)
                unset(static_san_flags)
            endif()
        endif()

        if(Options_INTERFACE)
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                target_compile_options(${Options_NAME} INTERFACE -Wno-non-template-friend)
            endif()

            target_compile_options(${Options_NAME} INTERFACE -Wno-missing-braces)
        endif()

        if(Options_TESTS)
            target_compile_options(${Options_NAME} PRIVATE -fno-omit-frame-pointer)

            # -Wno-redundant-move: want `std::move(std::as_const(x))` to be compiled without warnings in unit tests.
            target_compile_options(${Options_NAME} PRIVATE -Wno-redundant-move)

            if(LIBFN_COVERAGE)
                add_code_coverage_to_target(${Options_NAME} PRIVATE)

                # Some options may be redundant, but that's OK.
                target_compile_options(${Options_NAME} PRIVATE -O0)

                if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                    target_compile_options(${Options_NAME} PRIVATE
                        -fno-inline-small-functions
                        -fno-default-inline
                        -fno-early-inlining
                        -fno-aggressive-loop-optimizations
                        -fno-peephole
                        -fno-unroll-loops
                    )

                    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
                        target_compile_options(${Options_NAME} PRIVATE -funreachable-traps)
                    endif()
                endif()
            endif()
        endif()
    endif()
endfunction()
