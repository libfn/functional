find_package(Doxygen REQUIRED)

set(DOXYGEN_GENERATE_HTML NO)
set(DOXYGEN_GENERATE_XML YES)

doxygen_add_docs(
    docs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Generate documentation"
)
