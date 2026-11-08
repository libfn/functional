# Copyright (c) 2026 Bronek Kozicki
#
# Distributed under the ISC License. See accompanying file LICENSE.md
# or copy at https://opensource.org/licenses/ISC

# Compiler launcher for reject tests: runs the compile command following `--` and fails whenever it
# fails, printing the marker a reject test requires, "The compiler rejected the source", only if the
# compiler reported a non-fatal error at a source line.
# Usage: cmake -P RejectLauncher.cmake -- <compiler> [<argument>...]

cmake_minimum_required(VERSION 3.25)

# Bracket arguments forward each argument verbatim; expanding a list would split an argument
# containing `;` and join arguments following an unbalanced `[`
set(command "")
set(forwarding FALSE)
math(EXPR last "${CMAKE_ARGC} - 1")
foreach(index RANGE ${last})
    set(argument "${CMAKE_ARGV${index}}")
    if(forwarding)
        string(FIND "${argument}" "]=]" clash)
        if(NOT clash EQUAL -1)
            message(FATAL_ERROR "Cannot forward an argument containing `]=]`: ${argument}")
        endif()
        string(APPEND command " [=[${argument}]=]")
    elseif(argument STREQUAL "--")
        set(forwarding TRUE)
    endif()
endforeach()

# The matching below reads English diagnostics. GCC translates them, its error and internal compiler
# error labels included, through gettext, which reads LANGUAGE ahead of LC_ALL and LANG.
set(ENV{LANGUAGE} en)

cmake_language(EVAL CODE "execute_process(COMMAND ${command} RESULT_VARIABLE result
    OUTPUT_VARIABLE output ERROR_VARIABLE output ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE)")
if(NOT result MATCHES "^[0-9]+$")
    message(FATAL_ERROR "The compiler did not run to completion: ${result}")
elseif(result EQUAL 0)
    return()
endif()

# A diagnosed error names its source line, as `file:line[:column]: error:` (GCC, Clang) or
# `file(line[,column]): error[ <code>]:` (MSVC, clang-cl). A failing driver, such as GCC reporting its killed
# front end, names no line. A fatal or internal compiler error names one too but does not count: it
# reports a broken build, a compiler limit or a compiler failure, not the rejection a scenario tests.
string(ASCII 27 escape)
string(REGEX REPLACE "${escape}\\[[0-9;]*[A-Za-z]" "" output "${output}")
if(output MATCHES "(^|\n)[^\n]+(:[0-9]+(:[0-9]+)?: |\\([0-9]+(,[0-9]+)?\\) ?: )error( C[0-9]+)?:")
    message(FATAL_ERROR "The compiler rejected the source")
endif()
message(FATAL_ERROR "The compiler failed without reporting a non-fatal error at a source line")
