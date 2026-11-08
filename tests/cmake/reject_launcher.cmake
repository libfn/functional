# Copyright (c) 2026 Bronek Kozicki
#
# Distributed under the ISC License. See accompanying file LICENSE.md
# or copy at https://opensource.org/licenses/ISC

# Usage: cmake -DLAUNCHER=<RejectLauncher.cmake> -DMARKER=<marker> -P reject_launcher.cmake

cmake_minimum_required(VERSION 3.25)

# With LIBFN_REJECT_DIAGNOSTIC in its environment, this script stands in for a compiler which prints
# it and fails; CMake's own error line does not take the shape of a diagnostic. The environment
# carries the diagnostic because a -D argument, if the launcher mangled it, could start a configure
# run in the current directory. With LIBFN_REJECT_TRANSLATION as well, it stands in for GCC with a
# message catalog, printing the translation unless LANGUAGE selects English.
if(DEFINED ENV{LIBFN_REJECT_DIAGNOSTIC})
    if(DEFINED ENV{LIBFN_REJECT_TRANSLATION} AND NOT "$ENV{LANGUAGE}" MATCHES "^en")
        message(NOTICE "$ENV{LIBFN_REJECT_TRANSLATION}")
    else()
        message(NOTICE "$ENV{LIBFN_REJECT_DIAGNOSTIC}")
    endif()
    message(FATAL_ERROR "")
endif()

set(launch "${CMAKE_COMMAND}" -P "${LAUNCHER}" --)

function(launch_failing diagnostic)
    set(ENV{LIBFN_REJECT_DIAGNOSTIC} "${diagnostic}")
    if(ARGC GREATER 1)
        set(ENV{LIBFN_REJECT_TRANSLATION} "${ARGV1}")
    endif()
    execute_process(
        COMMAND ${launch} "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
    unset(ENV{LIBFN_REJECT_DIAGNOSTIC})
    unset(ENV{LIBFN_REJECT_TRANSLATION})
    set(result "${result}" PARENT_SCOPE)
    set(output "${output}" PARENT_SCOPE)
endfunction()

string(ASCII 27 escape)
foreach(diagnostic IN ITEMS
        "source.cpp:12:3: error: rejected"
        "source.cpp:12: error: rejected"
        "source.cpp:3:10: fatal error: missing.hpp: No such file or directory"
        "source.cpp(12): error C2672: rejected"
        "source.cpp(12,3): error: rejected"
        "source.cpp:12:3: error: rejected\nsource.cpp:20:1: internal compiler error: Segmentation fault"
        "${escape}[01m${escape}[Ksource.cpp:12:3:${escape}[m${escape}[K ${escape}[01;31m${escape}[Kerror: ${escape}[m${escape}[Krejected")
    launch_failing("${diagnostic}")
    if(result EQUAL 0 OR NOT output MATCHES "${MARKER}")
        message(SEND_ERROR "An error at a source line must fail the launch with the marker, got ${result}:\n${output}")
    endif()
endforeach()

foreach(diagnostic IN ITEMS
        "g++: fatal error: Killed signal terminated program cc1plus"
        "source.cpp:12:3: internal compiler error: Segmentation fault"
        "source.cpp(61,1): fatal error C1001: Internal compiler error.\n(compiler file 'msc1.cpp', line 1589)"
        "source.cpp(12): fatal error C1001: An internal error has occurred in the compiler."
        "cc1plus: error: unrecognized command-line option '-fnone'"
        "source.cpp:12:3: warning: unused variable 'error'")
    launch_failing("${diagnostic}")
    if(result EQUAL 0 OR output MATCHES "${MARKER}")
        message(SEND_ERROR "A failure without an error at a source line must fail the launch without the marker, got ${result}:\n${output}")
    endif()
endforeach()

# A user's language must not change the verdict: German translates the error label, and Spanish turns
# the internal compiler error label into one starting with "error"
set(ENV{LANGUAGE} de)
launch_failing("source.cpp:12:3: error: rejected" "source.cpp:12:3: Fehler: abgelehnt")
if(result EQUAL 0 OR NOT output MATCHES "${MARKER}")
    message(SEND_ERROR "An error at a source line must fail the launch with the marker in any language, got ${result}:\n${output}")
endif()
set(ENV{LANGUAGE} es)
launch_failing("source.cpp:12:3: internal compiler error: Segmentation fault"
    "source.cpp:12:3: error interno del compilador: Segmentation fault")
if(result EQUAL 0 OR output MATCHES "${MARKER}")
    message(SEND_ERROR "An internal compiler error must fail the launch without the marker in any language, got ${result}:\n${output}")
endif()
unset(ENV{LANGUAGE})

execute_process(COMMAND ${launch} "${CMAKE_COMMAND}" -E echo "a;b" "[c" "d]"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT result EQUAL 0 OR NOT output STREQUAL "a;b [c d]")
    message(SEND_ERROR "A succeeding command must succeed with its arguments intact, got ${result}:\n${output}")
endif()

execute_process(COMMAND ${launch} "${CMAKE_CURRENT_LIST_DIR}/no-such-compiler"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(result EQUAL 0 OR output MATCHES "${MARKER}")
    message(SEND_ERROR "A command that cannot start must fail the launch without the marker, got ${result}:\n${output}")
endif()
