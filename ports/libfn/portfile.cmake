set(VCPKG_BUILD_TYPE release)

# This overlay builds a local checkout; use the registry port for releases.
if(NOT DEFINED ENV{VCPKG_LIBFN_SOURCE_PATH})
    message(FATAL_ERROR
        "Set VCPKG_LIBFN_SOURCE_PATH to a libfn source checkout to use this overlay. "
        "To install a released version, use the vcpkg registry port.")
endif()
set(SOURCE_PATH "$ENV{VCPKG_LIBFN_SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DLIBFN_TESTS=OFF
        -DDISABLE_CCACHE_DETECTION=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/libfn)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/doc")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
