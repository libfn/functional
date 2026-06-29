// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "catch2/catch_test_macros.hpp"
#ifndef PFN_TEST_VALIDATION
#include <pfn/utility.hpp>
using pfn::unreachable;
#else
#include <utility>
using std::unreachable;
#endif

#include <type_traits>

TEST_CASE("unreachable", "[utility][polyfill][unreachable]")
{
  SECTION("expected signature")
  {
    static_assert(std::is_same_v<void, decltype(unreachable())>);
    static_assert(not noexcept(unreachable()));
    [[maybe_unused]] constexpr auto _signature_check = +[]() -> void { unreachable(); };
    SUCCEED();
  }
}
