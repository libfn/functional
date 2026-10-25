# Copyright (c) 2026 Bronek Kozicki
#
# Distributed under the ISC License. See accompanying file LICENSE.md
# or copy at https://opensource.org/licenses/ISC

include(CompilationOptions)

set(REJECT_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/RejectLauncher.cmake")
# Only Makefile and Ninja generators run compiler launchers. Targets using this one replace any
# global launcher, such as ccache, whose own failure would pass as a rejection.
if(CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
    set(REJECT_LAUNCH "${CMAKE_COMMAND}" -P "${REJECT_LAUNCHER}" --)
endif()
# RejectLauncher.cmake prints this exact text
set(REJECT_MARKER "The compiler rejected the source")

add_custom_target(tests_reject_controls)
add_test(
    NAME tests_reject_controls
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --target tests_reject_controls
)

# Scenario tests require this fixture, which other setup tests may join. A rejection counts only
# once each reject source compiles without a scenario, through the same launcher, if any, as its
# scenarios. Building the controls first also regenerates a stale build system once, rather than in
# each scenario build.
set_tests_properties(tests_reject_controls PROPERTIES FIXTURES_SETUP reject_setup)

# Generate an object target for SOURCE, which must compile, and for each
# `#ifdef LIBFN_REJECT_<scenario>` guard in it a test building SOURCE with only that guard defined,
# which the compiler must reject
function(add_reject_tests)
    set(oneValueArgs SOURCE MODE)
    set(multiValueArgs DEPENDENCIES LABELS)
    cmake_parse_arguments(Reject "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(path "${CMAKE_CURRENT_SOURCE_DIR}/${Reject_SOURCE}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
    file(STRINGS "${path}" guards REGEX "^#ifdef LIBFN_REJECT_[A-Za-z0-9_]+$")
    file(STRINGS "${path}" directives REGEX "^[ \t]*#.*LIBFN_REJECT_")
    if(NOT guards OR NOT guards STREQUAL directives)
        message(FATAL_ERROR "${Reject_SOURCE}: needs at least one guard, each spelled `#ifdef LIBFN_REJECT_<scenario>`")
    endif()
    list(TRANSFORM guards REPLACE "^#ifdef LIBFN_REJECT_" "" OUTPUT_VARIABLE scenarios)

    string(REGEX REPLACE "\\.cpp$" "" root_name "${Reject_SOURCE}")
    string(REPLACE "/" "_" root_name "${root_name}")
    set(control "tests_${root_name}_cxx${Reject_MODE}")
    add_library("${control}" OBJECT "${Reject_SOURCE}")
    add_dependencies("cxx${Reject_MODE}" "${control}")
    add_dependencies("tests" "${control}")
    set(targets "${control}")

    if(REJECT_LAUNCH)
        # Positive control for the scenarios, compiled through their launcher; the plain control
        # above keeps any global launcher instead
        set(launched "${control}_launched")
        add_library("${launched}" OBJECT EXCLUDE_FROM_ALL "${Reject_SOURCE}")
        set_property(TARGET "${launched}" PROPERTY CXX_COMPILER_LAUNCHER ${REJECT_LAUNCH})
        add_dependencies(tests_reject_controls "${launched}")
        # The build tracks the launcher's path, not its content; as a source property, this also
        # recompiles the plain control and the scenarios when the launcher changes
        set_property(SOURCE "${Reject_SOURCE}" PROPERTY OBJECT_DEPENDS "${REJECT_LAUNCHER}")
        list(APPEND targets "${launched}")
    else()
        add_dependencies(tests_reject_controls "${control}")
    endif()

    foreach(scenario IN LISTS scenarios)
        set(target "tests_${root_name}_${scenario}_cxx${Reject_MODE}")
        add_library("${target}" OBJECT EXCLUDE_FROM_ALL "${Reject_SOURCE}")
        target_compile_definitions("${target}" PRIVATE "LIBFN_REJECT_${scenario}")

        add_test(
            NAME "${target}"
            COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --target "${target}"
        )
        set_tests_properties("${target}" PROPERTIES
            FIXTURES_REQUIRED reject_setup
            LABELS "${Reject_LABELS};cxx${Reject_MODE};${root_name}")
        if(REJECT_LAUNCH)
            set_property(TARGET "${target}" PROPERTY CXX_COMPILER_LAUNCHER ${REJECT_LAUNCH})
            set_tests_properties("${target}" PROPERTIES PASS_REGULAR_EXPRESSION "${REJECT_MARKER}")
        else()
            # Any failed build passes here, even one failed by a concurrent build in the same tree
            set_tests_properties("${target}" PROPERTIES WILL_FAIL TRUE RESOURCE_LOCK build_tree)
        endif()
        list(APPEND targets "${target}")

        unset(target)
    endforeach()

    foreach(target IN LISTS targets)
        target_link_libraries("${target}" PRIVATE ${Reject_DEPENDENCIES})
        append_compilation_options("${target}" WARNINGS)
        set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${Reject_MODE}")
    endforeach()
endfunction()
