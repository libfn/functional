find_package(Doxygen REQUIRED)
find_package(Znai REQUIRED)

set(DOXYGEN_GENERATE_HTML NO)
set(DOXYGEN_GENERATE_XML YES)

macro(znai_export_docs TARGET SOURCE_DIR DEPLOY_DIR)
add_custom_target(${TARGET}
COMMAND ${Znai} --source ${SOURCE_DIR} --deploy ${DEPLOY_DIR} --doc-id '""' --lookup-paths ${CMAKE_BINARY_DIR}
COMMENT "Exporting documentation to ${DEPLOY_DIR}"
  )
endmacro()

doxygen_add_docs(
    docs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Generate documentation"
)

znai_export_docs(
  export_docs
  ${CMAKE_CURRENT_SOURCE_DIR}/docs
  ${CMAKE_BINARY_DIR}/docs
  COMMENT "Export final documentation"
)
  
add_dependencies(export_docs docs)
