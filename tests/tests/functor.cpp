// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functor.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

using namespace util;

namespace {
constexpr inline struct dummy_t final {
  auto operator()(auto &&fn) const noexcept -> fn::functor<dummy_t, decltype(fn)> { return {FWD(fn)}; }

  struct apply final {
    static auto operator()(fn::some_monadic_type auto &&v, auto &&fn) noexcept -> decltype(auto)
      requires requires { fn(v.value()); }
    {
      return FWD(v).transform([&fn](auto &&v) noexcept { return FWD(fn)(FWD(v)); });
    }
  };
} dummy = {};

constexpr auto fn1 = [](int i) constexpr -> int { return i + 1; };
constexpr auto fn2 = []() constexpr -> int { return 1; };

namespace check_expected {
using operand_t = fn::expected<int, bool>;
using is = static_check<dummy_t, operand_t>::bind;

static_assert(is::invocable_with_any(fn1));
static_assert(is::not_invocable_with_any(fn2)); // arity mismatch
} // namespace check_expected

namespace check_optional {
using operand_t = fn::optional<int>;
using is = static_check<dummy_t, operand_t>::bind;

static_assert(is::invocable_with_any(fn1));
static_assert(is::not_invocable_with_any(fn2)); // arity mismatch
} // namespace check_optional
} // namespace

TEST_CASE("user-defined monadic operation", "[functor]")
{
  CHECK((fn::expected<int, std::runtime_error>{12} | dummy(fn1)).value() == 13);
  CHECK((fn::optional{42} | dummy(fn1)).value() == 43);
}
