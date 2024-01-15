// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/functor.hpp"

namespace {
constexpr inline struct dummy_t final {
  auto operator()(auto &&fn) const noexcept
      -> fn::functor<dummy_t, decltype(fn)>
  {
    return {std::forward<decltype(fn)>(fn)};
  }

  struct apply final {
    static auto operator()(fn::some_monadic_type auto &&v, auto &&fn) noexcept
        -> decltype(auto)
      requires requires { fn(v.value()); }
    {
      return std::forward<decltype(v)>(v).transform([&fn](auto &&v) noexcept {
        return std::forward<decltype(fn)>(fn)(std::forward<decltype(v)>(v));
      });
    }
  };
} dummy = {};

} // namespace

namespace fn {
constexpr auto fn1 = [](int i) constexpr -> int { return i + 1; };
static_assert(
    monadic_invocable<dummy_t, std::expected<int, bool>, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, std::optional<int>, decltype(fn1)>);
static_assert(
    monadic_invocable<dummy_t, std::expected<int, bool> &, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, std::optional<int> &, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, std::expected<int, bool> const &,
                                decltype(fn1)>);
static_assert(
    monadic_invocable<dummy_t, std::optional<int> const &, decltype(fn1)>);
static_assert(
    monadic_invocable<dummy_t, std::expected<int, bool> &&, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, std::optional<int> &&, decltype(fn1)>);
static_assert(monadic_invocable<dummy_t, std::expected<int, bool> const &&,
                                decltype(fn1)>);
static_assert(
    monadic_invocable<dummy_t, std::optional<int> const &&, decltype(fn1)>);

constexpr auto fn2 = []() constexpr -> int { return 1; };
static_assert(
    !monadic_invocable<dummy_t, std::expected<int, bool>, decltype(fn2)>);
static_assert(!monadic_invocable<dummy_t, std::optional<int>, decltype(fn2)>);

static_assert(some_expected<std::expected<int, bool>>);
static_assert(some_expected<std::expected<int, bool> const>);
static_assert(some_expected<std::expected<int, bool> &>);
static_assert(some_expected<std::expected<int, bool> const &>);
static_assert(some_expected<std::expected<int, bool> &&>);
static_assert(some_expected<std::expected<int, bool> const &&>);

static_assert(some_optional<std::optional<int>>);
static_assert(some_optional<std::optional<int> const>);
static_assert(some_optional<std::optional<int> &>);
static_assert(some_optional<std::optional<int> const &>);
static_assert(some_optional<std::optional<int> &&>);
static_assert(some_optional<std::optional<int> const &&>);

static_assert(some_monadic_type<std::expected<int, bool>>);
static_assert(some_monadic_type<std::expected<int, bool> const>);
static_assert(some_monadic_type<std::expected<int, bool> &>);
static_assert(some_monadic_type<std::expected<int, bool> const &>);
static_assert(some_monadic_type<std::expected<int, bool> &&>);
static_assert(some_monadic_type<std::expected<int, bool> const &&>);
static_assert(some_monadic_type<std::optional<int>>);
static_assert(some_monadic_type<std::optional<int> const>);
static_assert(some_monadic_type<std::optional<int> &>);
static_assert(some_monadic_type<std::optional<int> const &>);
static_assert(some_monadic_type<std::optional<int> &&>);
static_assert(some_monadic_type<std::optional<int> const &&>);
} // namespace fn
