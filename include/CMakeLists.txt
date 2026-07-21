cmake_minimum_required(VERSION 3.25)
project(include)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CompilationOptions)
include(TargetGenerator)

### include/libfn_version.hpp

set(INCLUDE_LIBFN_VERSION_HEADERS
    libfn_version.hpp
)

add_library(include_libfn_version INTERFACE)
target_sources(include_libfn_version INTERFACE
    FILE_SET include_libfn_version_headers
    TYPE HEADERS
    FILES ${INCLUDE_LIBFN_VERSION_HEADERS})
append_compilation_options(include_libfn_version INTERFACE)
target_include_directories(include_libfn_version SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(include_libfn_version PROPERTIES EXPORT_NAME version)
target_compile_features(include_libfn_version INTERFACE cxx_std_20)

install(TARGETS include_libfn_version
    EXPORT libfnTargets
    FILE_SET include_libfn_version_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Generate sentinel target for the header, as a basic sanity check
set(mode 20)
set(target "sentinel_libfn_version_cxx${mode}")

create_target_for_file(
    NAME "${target}"
    SOURCE "libfn_version.hpp"
    NEW_SOURCE "#include <libfn_version.hpp>\n#ifndef LIBFN_VERSION\n#error LIBFN_VERSION not defined\n#endif\nint main() {}\n"
    SOURCE_ROOT "${CMAKE_BINARY_DIR}/sentinel"
    DEPENDENCIES include_libfn_version
)
append_compilation_options("${target}" WARNINGS)
add_dependencies("sentinels" "${target}")
set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")

unset(target)
unset(mode)

### include/pfn

# Pls keep the filenames sorted
set(INCLUDE_PFN_HEADERS
    pfn/expected.hpp
    pfn/functional.hpp
    pfn/optional.hpp
    pfn/tuple.hpp
    pfn/utility.hpp
)

add_library(include_pfn INTERFACE)
target_sources(include_pfn INTERFACE
    FILE_SET include_pfn_headers
    TYPE HEADERS
    FILES ${INCLUDE_PFN_HEADERS})
append_compilation_options(include_pfn INTERFACE)
target_include_directories(include_pfn SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(include_pfn PROPERTIES EXPORT_NAME pfn)
target_link_libraries(include_pfn INTERFACE include_libfn_version)
target_compile_features(include_pfn INTERFACE cxx_std_20)

install(TARGETS include_pfn
    EXPORT libfnTargets
    FILE_SET include_pfn_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Generate sentinel target for each individual header, as a basic sanity check
foreach(source IN ITEMS ${INCLUDE_PFN_HEADERS})
    string(REGEX REPLACE "^(pfn)/(detail|)/?([^\.]+)\.hpp$" "\\1_\\2\\3" root_name ${source})
    set(mode 20)
    set(target "sentinel_${root_name}_cxx${mode}")

    create_target_for_file(
        NAME "${target}"
        SOURCE "${source}"
        NEW_SOURCE "#include <${source}>\n#ifdef FWD\n#error FWD leaked from ${source}\n#endif\n#ifdef DEDUCED_RETURN\n#error DEDUCED_RETURN leaked from ${source}\n#endif\n#ifdef ASSERT\n#error ASSERT leaked from ${source}\n#endif\nint main() {}\n"
        SOURCE_ROOT "${CMAKE_BINARY_DIR}/sentinel"
        DEPENDENCIES include_pfn
    )
    append_compilation_options("${target}" WARNINGS)
    add_dependencies("sentinels" "${target}")
    set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")

    unset(target)
    unset(mode)
    unset(root_name)
endforeach()

### include/fn

# Pls keep the filenames sorted
set(INCLUDE_FN_HEADERS
    fn/detail/functional.hpp
    fn/detail/fwd.hpp
    fn/detail/macro_begin.hpp
    fn/detail/macro_end.hpp
    fn/detail/meta.hpp
    fn/detail/monadic.hpp
    fn/detail/pack_impl.hpp
    fn/detail/traits.hpp
    fn/detail/variadic_union.hpp
    fn/and_then.hpp
    fn/choice.hpp
    fn/concepts.hpp
    fn/copack.hpp
    fn/discard.hpp
    fn/expected.hpp
    fn/fail.hpp
    fn/filter.hpp
    fn/functional.hpp
    fn/functor.hpp
    fn/fwd.hpp
    fn/inspect_error.hpp
    fn/inspect.hpp
    fn/just.hpp
    fn/monadic.hpp
    fn/optional.hpp
    fn/or_else.hpp
    fn/pack.hpp
    fn/recover.hpp
    fn/transform_error.hpp
    fn/transform.hpp
    fn/utility.hpp
    fn/value_or.hpp
)

add_library(include_fn INTERFACE)
target_sources(include_fn INTERFACE
    FILE_SET include_fn_headers
    TYPE HEADERS
    FILES ${INCLUDE_FN_HEADERS})
append_compilation_options(include_fn INTERFACE)
target_include_directories(include_fn SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(include_fn PROPERTIES EXPORT_NAME fn)
target_link_libraries(include_fn INTERFACE include_pfn include_libfn_version)
target_compile_features(include_fn INTERFACE cxx_std_20)

install(TARGETS include_fn
    EXPORT libfnTargets
    FILE_SET include_fn_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Generate sentinel target for each individual header, as a basic sanity check
foreach(source IN ITEMS ${INCLUDE_FN_HEADERS})
    # A lone macro_begin.hpp TU leaves unbalanced pushes and would trip the leak check below
    if(source MATCHES "fn/detail/macro_(begin|end)\.hpp")
        continue()
    endif()
    string(REGEX REPLACE "^(fn)/(detail|)/?([^\.]+)\.hpp$" "\\1_\\2\\3" root_name ${source})
    set(mode 20)
    set(target "sentinel_${root_name}_cxx${mode}")

    create_target_for_file(
        NAME "${target}"
        SOURCE "${source}"
        NEW_SOURCE "#include <${source}>\n#ifdef FWD\n#error FWD leaked from ${source}\n#endif\n#ifdef DEDUCED_RETURN\n#error DEDUCED_RETURN leaked from ${source}\n#endif\n#ifdef ASSERT\n#error ASSERT leaked from ${source}\n#endif\nint main() {}\n"
        SOURCE_ROOT "${CMAKE_BINARY_DIR}/sentinel"
        DEPENDENCIES include_fn
    )
    append_compilation_options("${target}" WARNINGS)
    add_dependencies("sentinels" "${target}")
    set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")

    unset(target)
    unset(mode)
    unset(root_name)
endforeach()

### fn_cxx26

# Alternative entry point: fn with the C++26 mode selected; a consumer target
# links exactly one of libfn::fn / libfn::fn_cxx26. Owns no headers.
add_library(include_fn_cxx26 INTERFACE)
set_target_properties(include_fn_cxx26 PROPERTIES EXPORT_NAME fn_cxx26)
target_link_libraries(include_fn_cxx26 INTERFACE include_fn)
target_compile_definitions(include_fn_cxx26 INTERFACE LIBFN_CXX26)
target_compile_features(include_fn_cxx26 INTERFACE cxx_std_26)

install(TARGETS include_fn_cxx26
    EXPORT libfnTargets)

# Sentinel target for include_fn_cxx26 to test the propagation of LIBFN_CXX26 define
if("cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(mode 26)
    set(target "sentinel_fn_cxx26_cxx${mode}")

    create_target_for_file(
        NAME "${target}"
        SOURCE "libfn_version.hpp"
        NEW_SOURCE "#include <libfn_version.hpp>\n#ifndef LIBFN_CXX26\n#error LIBFN_CXX26 not defined\n#endif\nint main() {}\n"
        # Own SOURCE_ROOT: the generated TU name derives from SOURCE, shared with the libfn_version sentinel
        SOURCE_ROOT "${CMAKE_BINARY_DIR}/sentinel/fn_cxx26"
        DEPENDENCIES include_fn_cxx26
    )
    append_compilation_options("${target}" WARNINGS)
    add_dependencies("sentinels" "${target}")
    set_property(TARGET "${target}" PROPERTY CXX_STANDARD "${mode}")

    unset(target)
    unset(mode)
endif()
