find_package(Doxygen REQUIRED)

doxygen_add_docs(
    docs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Generate documentation"
)
