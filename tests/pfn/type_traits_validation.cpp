// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <version>

#if defined(__cpp_lib_reference_from_temporary) && __cpp_lib_reference_from_temporary >= 202202L

#include <type_traits>

namespace subject = std;

#define PFN_TEST_VALIDATION
#define PFN_TEST_NESTED
#include "type_traits.cpp"

#else

#include "catch2/catch_test_macros.hpp"

// Placeholder so the binary is not empty; Catch2 fails when no tests run. The traits are C++23,
// so the C++20 lanes, and standard libraries without them, have nothing to validate against.
TEST_CASE("type_traits validation", "[type_traits][validation]") //
{
  SUCCEED("type_traits validation is skipped without std::reference_constructs_from_temporary");
}

#endif
