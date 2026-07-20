// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/optional.hpp>
#include <libfn_version.hpp>
#include <pfn/expected.hpp>

#include <catch2/catch_all.hpp>

#include <type_traits>

TEST_CASE("LIBFN_VERSION inline namespace", "[libfn_version]")
{
  static_assert(std::is_same_v<fn::LIBFN_VERSION::optional<int>, fn::optional<int>>);
  static_assert(std::is_same_v<pfn::LIBFN_VERSION::expected<int, bool>, pfn::expected<int, bool>>);
  SUCCEED();
}
