# Enable coverate report target and compilation options, requires gcovr

# In this project, 100% tests coverage does not actually mean much, because the most useful
# test cases are around overload resolution, parameter dispatch, value categories,
# concepts etc. elements of the language which may not show in the coverage tests.
# This means that we need many more tests than just the minimum needed to ensure "good" coverage.

set(CODE_COVERAGE_VERBOSE ON)
set(CODE_COVERAGE_FORMAT "xml" CACHE STRING "Format of the coverage report.")
# --merge-lines requires gcovr 8.4 or newer.
set(GCOVR_ADDITIONAL_ARGS
--merge-lines
--exclude-noncode-lines
--exclude-unreachable-branches
--exclude-throw-branches
--print-summary
--gcov-ignore-errors=no_working_dir_found)

setup_target_for_coverage_gcovr(
    NAME coverage
    FORMAT ${CODE_COVERAGE_FORMAT}
    EXCLUDE "tests" "examples" "${PROJECT_BINARY_DIR}/_deps"
    DEPENDENCIES tests
)
