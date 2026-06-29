set(VCPKG_BUILD_TYPE release)

if(DEFINED ENV{VCPKG_LIBFN_SOURCE_PATH})
    set(SOURCE_PATH "$ENV{VCPKG_LIBFN_SOURCE_PATH}")
else()
    # TODO replace with vcpkg_from_github(... REF "v${VERSION}" SHA512 <pinned>)
    # once the first release is tagged. Until then this port is overlay-only.
    message(FATAL_ERROR
        "VCPKG_LIBFN_SOURCE_PATH must point to a libfn source checkout. "
        "Fetching by tag from GitHub is not yet supported (no tagged release).")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DLIBFN_TESTS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/libfn)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/doc")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
