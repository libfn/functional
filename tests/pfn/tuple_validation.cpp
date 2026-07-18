// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <version>

#if defined(__cpp_lib_apply) && __cpp_lib_apply >= 202603L

#include <tuple>
#include <type_traits>

namespace subject = std;

#define PFN_TEST_VALIDATION
#define PFN_TEST_NESTED
#include "tuple.cpp"

#else

#include "catch2/catch_test_macros.hpp"

// Placeholder so the binary is not empty; Catch2 fails when no tests run. P1317R2's traits carry
// __cpp_lib_apply 202603L; no standard library ships them yet, and the C++23 validation modes
// never will - this activates per stdlib once a C++26 validation lane exists.
TEST_CASE("tuple validation", "[tuple][validation]") //
{
  SUCCEED("tuple validation is skipped without P1317R2 library support");
}

#endif
