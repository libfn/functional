// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/functor.hpp"
#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

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

} // namespace

constexpr auto fn1 = [](int i) constexpr -> int { return i + 1; };

namespace fn {
static_assert(monadic_invocable<dummy_t, expected<int, bool>, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, optional<int>, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, expected<int, bool> &, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, optional<int> &, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, expected<int, bool> const &, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, optional<int> const &, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, expected<int, bool> &&, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, optional<int> &&, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, expected<int, bool> const &&, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, optional<int> const &&, decltype(fn1)>);

constexpr auto fn2 = []() constexpr -> int { return 1; };
static_assert(not monadic_invocable<dummy_t, expected<int, bool>,
                                    decltype(fn2)>); // arity mismatch
static_assert(not monadic_invocable<dummy_t, optional<int>,
                                    decltype(fn2)>); // arity mismatch
} // namespace fn

TEST_CASE("user-defined monadic operation", "[functor]")
{
  CHECK((fn::expected<int, std::runtime_error>{12} | dummy(fn1)).value() == 13);
  CHECK((fn::optional{42} | dummy(fn1)).value() == 43);
}
