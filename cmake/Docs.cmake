find_package(Doxygen REQUIRED)
find_package(Znai REQUIRED)

set(DOXYGEN_GENERATE_HTML NO)
set(DOXYGEN_GENERATE_XML YES)

# Doxygen reads a staged copy of include/ with the ABI inline namespace stripped, so the XML
# carries the API exactly as readers spell it (fn::X, pfn::X), independent of doxygen's and
# znai's inline-namespace handling.
set(docs_staged_include ${CMAKE_BINARY_DIR}/docs_include)
file(MAKE_DIRECTORY ${docs_staged_include})
add_custom_target(docs_stage_include
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/include
        -DDEST_DIR=${docs_staged_include}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StripNamespaceWrap.cmake
    COMMENT "Stage include/ with the ABI namespace stripped"
)
set(DOXYGEN_STRIP_FROM_PATH ${docs_staged_include})

macro(znai_export_docs TARGET SOURCE_DIR DEPLOY_DIR)
    add_custom_target(
        ${TARGET}
        COMMAND ${Znai} --source ${SOURCE_DIR} --deploy ${DEPLOY_DIR} --doc-id '""' --lookup-paths ${CMAKE_BINARY_DIR}
        COMMENT "Exporting documentation to ${DEPLOY_DIR}"
    )
endmacro()

doxygen_add_docs(
    docs_xml
    ${docs_staged_include}
    COMMENT "Generate documentation"
)
add_dependencies(docs_xml docs_stage_include)

znai_export_docs(
    export_docs
    ${CMAKE_CURRENT_SOURCE_DIR}/docs
    ${CMAKE_BINARY_DIR}/docs
    COMMENT "Export final documentation"
)

add_dependencies(export_docs docs_xml)
