// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef PFN_TEST_VALIDATION
// TODO: Add death tests. Until then, empty definition to avoid false "no coverage" reports
#define LIBFN_ASSERT(...)
#include <pfn/optional.hpp>
using pfn::make_optional;
using pfn::optional;
#else
#include <optional>
using std::make_optional;
using std::optional;
#endif

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("dummy")
{
  SUCCEED(); // TODO
}
