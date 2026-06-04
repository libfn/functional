// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#if LIBFN_MODE >= 23

#include <expected>
using std::bad_expected_access;
using std::expected;
using std::unexpect;
using std::unexpect_t;
using std::unexpected;

#define PFN_TEST_VALIDATION
#define PFN_TEST_NESTED
#include "expected.cpp"

#else

#include "catch2/catch_test_macros.hpp"

// Placeholder so the C++20 binary is not empty; Catch2 fails when no tests run.
TEST_CASE("expected validation", "[expected][validation]") //
{
  SUCCEED("expected validation is skipped in C++20 mode");
}

#endif
