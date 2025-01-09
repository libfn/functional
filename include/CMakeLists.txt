cmake_minimum_required(VERSION 3.25)
project(include)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pls keep the filenames sorted
set(INCLUDE_FN_HEADERS
    fn/detail/functional.hpp
    fn/detail/fwd_macro.hpp
    fn/detail/fwd.hpp
    fn/detail/meta.hpp
    fn/detail/pack_impl.hpp
    fn/detail/traits.hpp
    fn/detail/variadic_union.hpp
    fn/and_then.hpp
    fn/choice.hpp
    fn/concepts.hpp
    fn/expected.hpp
    fn/fail.hpp
    fn/filter.hpp
    fn/functional.hpp
    fn/functor.hpp
    fn/fwd.hpp
    fn/inspect_error.hpp
    fn/inspect.hpp
    fn/optional.hpp
    fn/or_else.hpp
    fn/pack.hpp
    fn/recover.hpp
    fn/sum.hpp
    fn/transform_error.hpp
    fn/transform.hpp
    fn/utility.hpp
    fn/value_or.hpp
)

add_library(include_fn INTERFACE)
target_sources(include_fn INTERFACE
    FILE_SET include_fn_headers
    TYPE HEADERS
    FILES ${INCLUDE_FN_HEADERS})
target_include_directories(include_fn INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS include_fn
    FILE_SET include_fn_headers
    DESTINATION include)

include(TargetGenerator)

# Generate sentinel target for each individual header, as a basic sanity check
foreach(source IN ITEMS ${INCLUDE_FN_HEADERS})
    string(REGEX REPLACE "^fn/(detail|)/?([^\.]+)\.hpp$" "\\1\\2" root_name ${source})
    setup_target_for_file(
        NAME "include_fn_sentinel_${root_name}"
        SOURCE "${source}"
        NEW_SOURCE "#include <${source}>\nint main() {}\n"
        DEPENDENCIES include_fn
        COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wno-missing-braces
    )
    unset(root_name)

endforeach()
