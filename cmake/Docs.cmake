find_package(Doxygen REQUIRED)
find_package(Znai REQUIRED)

set(DOXYGEN_GENERATE_HTML NO)
set(DOXYGEN_GENERATE_XML YES)

# Expand LIBFN_VERSION so the XML carries the real inline-namespace name; the spelling is read
# from the header, the single source of truth.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/include/libfn_version.hpp libfn_version_defines
    REGEX "^#define LIBFN_VERSION ")
list(FILTER libfn_version_defines EXCLUDE REGEX "_cxx26$")
list(GET libfn_version_defines 0 libfn_version_define)
string(REGEX REPLACE "^#define LIBFN_VERSION " "" libfn_version_namespace "${libfn_version_define}")
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
set(DOXYGEN_PREDEFINED "LIBFN_VERSION=${libfn_version_namespace}")
unset(libfn_version_defines)
unset(libfn_version_define)
unset(libfn_version_namespace)

macro(znai_export_docs TARGET SOURCE_DIR DEPLOY_DIR)
    add_custom_target(
        ${TARGET}
        COMMAND ${Znai} --source ${SOURCE_DIR} --deploy ${DEPLOY_DIR} --doc-id '""' --lookup-paths ${CMAKE_BINARY_DIR}
        COMMENT "Exporting documentation to ${DEPLOY_DIR}"
    )
endmacro()

doxygen_add_docs(
    docs_xml
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Generate documentation"
)

znai_export_docs(
    export_docs
    ${CMAKE_CURRENT_SOURCE_DIR}/docs
    ${CMAKE_BINARY_DIR}/docs
    COMMENT "Export final documentation"
)

add_dependencies(export_docs docs_xml)
