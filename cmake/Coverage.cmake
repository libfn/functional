# Enable coverate report target and compilation options, requires gcovr

# In this project, 100% tests coverage does not actually mean much, because the most useful
# test cases are around overload resolution, parameter dispatch, value categories,
# concepts etc. elements of the language which may not show in the coverage tests.
# This means that we need many more tests than just the minimum needed to ensure "good" coverage.

set(CODE_COVERAGE_VERBOSE ON)
set(CODE_COVERAGE_FORMAT "xml" CACHE STRING "Format of the coverage report.")
# --merge-lines: gcovr emits one line record per template instantiation, and a consumer which does
# not merge them (sonarcloud) counts every record of a line some instantiation never ran as a miss.
# Requires gcovr 8.4 or newer.
set(GCOVR_ADDITIONAL_ARGS
--merge-lines
--exclude-noncode-lines
--exclude-unreachable-branches
--exclude-throw-branches
--print-summary
--gcov-ignore-errors=no_working_dir_found)

# Do not count fetched third-party sources (_deps) as project code.
setup_target_for_coverage_gcovr(
    NAME coverage
    FORMAT ${CODE_COVERAGE_FORMAT}
    EXCLUDE "tests" "examples" "${PROJECT_BINARY_DIR}"
    DEPENDENCIES tests
)
