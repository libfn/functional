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

    if(Options_INTERFACE)
        target_compile_options(${Options_NAME} INTERFACE
            $<$<CXX_COMPILER_ID:MSVC>:/permissive->
            $<$<CXX_COMPILER_ID:GNU>:-Wno-non-template-friend>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-missing-braces>)

        # MSVC's <eh.h> declares a global `unexpected` that shadows the std::expected/std::unexpected
        # vocabulary used by libfn; _HAS_CXX23 (MSVC STL's C++23-mode switch) drops the legacy declaration.
        # Must be INTERFACE because <eh.h> may be included before any libfn header.
        target_compile_definitions(${Options_NAME} INTERFACE
            $<$<CXX_COMPILER_ID:MSVC>:_HAS_CXX23>)
    endif()

    if(Options_WARNINGS)
        # MSVC: disable C4456 (decl hides previous local), C4244 (narrowing _Ty→_Ty),
        # C4101 (unref local var), C4805 (bool/int in heterogeneous == ).
        # Clang ≥19: -Wno-c2y-extensions allows Catch2's use of __COUNTER__ (not a C++ preprocessor feature).
        target_compile_options(${Options_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4>
            $<$<CXX_COMPILER_ID:MSVC>:/wd4456>
            $<$<CXX_COMPILER_ID:MSVC>:/wd4244>
            $<$<CXX_COMPILER_ID:MSVC>:/wd4101>
            $<$<CXX_COMPILER_ID:MSVC>:/wd4805>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
            $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19>>:-Wno-c2y-extensions>
        )
    endif()

    if(Options_OPTIMIZATION)
        target_compile_options(${Options_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:$<IF:$<CONFIG:Debug>,/Od,/O2>>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:$<IF:$<CONFIG:Debug>,-O0,-O2>>
            $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-fno-omit-frame-pointer>
        )

        if(LIBFN_SANITIZERS)
            # GCC's constexpr evaluator is incompatible with UBSan instrumentation in libstdc++, so GCC stays on
            # address (+leak). Clang/AppleClang gets address,undefined (+leak on non-Darwin).
            # Static-link the runtimes: GCC needs per-sanitizer flags; Clang takes the umbrella -static-libsan.
            # On macOS we support only AppleClang and libclang_rt is bundled as dylib, so no static-link flag.
            target_compile_options(${Options_NAME} PRIVATE
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fsanitize=address,leak>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,AppleClang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-fsanitize=address,undefined,leak>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:AppleClang>,$<PLATFORM_ID:Darwin>>:-fsanitize=address,undefined>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,14>>:-funreachable-traps>
            )
            target_link_options(${Options_NAME} PRIVATE
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fsanitize=address,leak>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,AppleClang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-fsanitize=address,undefined,leak>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:AppleClang>,$<PLATFORM_ID:Darwin>>:-fsanitize=address,undefined>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-static-libasan>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-static-liblsan>
                $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,AppleClang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-static-libsan>
            )
        endif()
    endif()

    if(Options_TESTS)
        # -Wno-redundant-move: allow std::move(std::as_const(x)) without warning in unit tests.
        target_compile_options(${Options_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-omit-frame-pointer>
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-redundant-move>
        )

        if(LIBFN_COVERAGE)
            add_code_coverage_to_target(${Options_NAME} PRIVATE)

            # Some options may be redundant, but that's OK.
            target_compile_options(${Options_NAME} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O0>
                $<$<CXX_COMPILER_ID:GNU>:-fno-inline-small-functions>
                $<$<CXX_COMPILER_ID:GNU>:-fno-default-inline>
                $<$<CXX_COMPILER_ID:GNU>:-fno-early-inlining>
                $<$<CXX_COMPILER_ID:GNU>:-fno-aggressive-loop-optimizations>
                $<$<CXX_COMPILER_ID:GNU>:-fno-peephole>
                $<$<CXX_COMPILER_ID:GNU>:-fno-unroll-loops>
                $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,14>>:-funreachable-traps>
            )
        endif()
    endif()
endfunction()
