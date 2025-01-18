// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <pfn/expected.hpp>

#include <catch2/catch_all.hpp>

enum Error { unknown };

TEST_CASE("expected_polyfill", "[expected][polyfill][cxx20compat]")
{
  using T = fn::detail::expected<int, Error>;
  constexpr auto a = sizeof(T);
  static_assert(a >= std::max(sizeof(Error), sizeof(int)));
  SUCCEED();
}
