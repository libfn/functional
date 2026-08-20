find_package(Doxygen REQUIRED)
find_package(Znai REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(DOXYGEN_GENERATE_HTML NO)
set(DOXYGEN_GENERATE_XML YES)

# doxygen records a trailing return type verbatim, so DEDUCED_RETURN would reach the API
# reference as itself. Expanding only what is named here substitutes the form every compiler
# but MSVC sees, and leaves every other macro to be read as written.
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
set(DOXYGEN_PREDEFINED "DEDUCED_RETURN(x)=decltype(auto)" "explicit(x)=explicit")

# Doxygen reads a staged copy of include/ with the ABI inline namespace stripped, so the XML
# carries the API exactly as readers spell it (fn::X, pfn::X), independent of doxygen's and
# znai's inline-namespace handling.
set(docs_staged_include ${CMAKE_BINARY_DIR}/docs_include)
file(MAKE_DIRECTORY ${docs_staged_include})
add_custom_target(docs_stage_include
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/include"
        "-DDEST_DIR=${docs_staged_include}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StripNamespaceWrap.cmake"
    COMMENT "Stage include/ with the ABI namespace stripped"
    VERBATIM
)
set(DOXYGEN_STRIP_FROM_PATH ${docs_staged_include})

macro(znai_export_docs TARGET SOURCE_DIR DEPLOY_DIR)
    set(docs_versioned "${DEPLOY_DIR}/v${LIBFN_PROJECT_VERSION}")
    add_custom_target(
        ${TARGET}
        # Start from an empty deploy dir: znai does not reliably clean it, and a stale root
        # entry would ship through the site assembly unguarded. The root pass writes the
        # deploy root itself, so it runs before the versioned pass writes beneath it.
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DEPLOY_DIR}"
        COMMAND ${Znai} --source ${SOURCE_DIR} --deploy ${DEPLOY_DIR} --doc-id '""' --lookup-paths ${CMAKE_BINARY_DIR}
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fix_site_urls.py" "${DEPLOY_DIR}"
        # Each release keeps its own copy of the site, served under /v<version>/. znai deploys
        # to <deploy>/<doc-id> and prefixes every internal URL with the doc id, so naming the
        # version directory in the doc id makes the copy self-consistent there.
        COMMAND ${Znai} --source ${SOURCE_DIR} --deploy ${DEPLOY_DIR} --doc-id "v${LIBFN_PROJECT_VERSION}" --lookup-paths ${CMAKE_BINARY_DIR}
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fix_site_urls.py" "${docs_versioned}" --doc-id "v${LIBFN_PROJECT_VERSION}"
        # The site carries the single-header artifact: Pages serves it with the permissive
        # CORS that Compiler Explorer's URL include needs and GitHub release assets lack.
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/amalgamate.py" -o "${DEPLOY_DIR}/libfn.hpp"
        COMMAND ${CMAKE_COMMAND} -E copy "${DEPLOY_DIR}/libfn.hpp" "${docs_versioned}/libfn.hpp"
        COMMENT "Exporting documentation to ${DEPLOY_DIR}"
    )
endmacro()

doxygen_add_docs(
    docs_xml
    ${docs_staged_include}
    COMMENT "Generate documentation"
)
add_dependencies(docs_xml docs_stage_include)

# znai reads a staged copy of docs/, which carries a chapter per root document alongside the
# API reference; the root documents themselves stay whole, and out of the site's source.
set(docs_staged_source ${CMAKE_BINARY_DIR}/docs_source)
add_custom_target(docs_stage_source
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/stage_docs_source.py"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${docs_staged_source}"
    COMMENT "Stage docs/ with a chapter per root document"
    VERBATIM
)

# A reference page presents an overload set the way cppreference does, as one listing of
# signatures - which znai cannot draw, its doxygen node carrying no ref-qualifier. The
# listings are therefore written out in the pages, and this holds them to the headers.
add_custom_target(docs_check_signatures
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/doxygen_signatures.py" check
        --xml "${CMAKE_BINARY_DIR}/xml"
        --docs "${CMAKE_CURRENT_SOURCE_DIR}/docs/reference"
    COMMENT "Check the reference signature listings against the headers"
    VERBATIM
)
add_dependencies(docs_check_signatures docs_xml)

# znai renders a description it cannot find as silence, so a page can lose one without the
# build noticing. This holds what the headers document to what the site actually reaches.
add_custom_target(docs_check_coverage
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_docs_coverage.py"
        --xml "${CMAKE_BINARY_DIR}/xml"
        --docs "${CMAKE_CURRENT_SOURCE_DIR}/docs"
    COMMENT "Check every documented entity reaches the documentation"
    VERBATIM
)
add_dependencies(docs_check_coverage docs_xml)

znai_export_docs(
    export_docs
    ${docs_staged_source}
    ${CMAKE_BINARY_DIR}/docs
    COMMENT "Export final documentation"
)

add_dependencies(export_docs docs_xml docs_stage_source docs_check_signatures docs_check_coverage)
