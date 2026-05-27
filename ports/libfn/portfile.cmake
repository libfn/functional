set(VCPKG_BUILD_TYPE release)

if(DEFINED ENV{VCPKG_LIBFN_SOURCE_PATH})
    set(SOURCE_PATH "$ENV{VCPKG_LIBFN_SOURCE_PATH}")
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO libfn/functional
        REF "v${VERSION}"
        SHA512 0
        HEAD_REF main
    )
endif()

set(DISABLE_CXX23 OFF)
if("disable-cxx23" IN_LIST FEATURES)
    set(DISABLE_CXX23 ON)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DLIBFN_TESTS=OFF
        -DDISABLE_CXX23=${DISABLE_CXX23}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/libfn)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/doc")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
