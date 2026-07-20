# Copyright (c) 2026 Bronek Kozicki
#
# Distributed under the ISC License. See accompanying file LICENSE.md
# or copy at https://opensource.org/licenses/ISC

# Stage a copy of the header tree with the ABI inline namespace stripped, for documentation:
# doxygen then sees the API exactly as readers spell it (fn::X, pfn::X), independent of any
# doxygen/znai handling of inline namespaces. The replaced spellings are exact literals because
# scripts/check_namespace_wrap.py enforces them in pre-commit.
# Usage: cmake -DSOURCE_DIR=<include dir> -DDEST_DIR=<staging dir> -P StripNamespaceWrap.cmake

foreach(var SOURCE_DIR DEST_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} must be set")
    endif()
endforeach()

file(GLOB_RECURSE headers RELATIVE "${SOURCE_DIR}" "${SOURCE_DIR}/*.hpp")
foreach(header IN LISTS headers)
    file(READ "${SOURCE_DIR}/${header}" content)
    string(REPLACE "inline namespace LIBFN_VERSION {\n" "" content "${content}")
    string(REPLACE "} // namespace LIBFN_VERSION\n" "" content "${content}")
    string(REPLACE "namespace fn::inline LIBFN_VERSION::detail {" "namespace fn::detail {" content "${content}")
    string(REPLACE "} // namespace fn::inline LIBFN_VERSION::detail" "} // namespace fn::detail" content "${content}")
    file(WRITE "${DEST_DIR}/${header}" "${content}")
endforeach()
