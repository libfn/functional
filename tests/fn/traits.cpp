// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/traits.hpp>

#include <fn/expected.hpp>
#include <fn/optional.hpp>
#include <fn/pack.hpp>
#include <fn/transform.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <utility>

// Incomplete functors must fail compilation for any operand; the suite cannot test this.

namespace {
struct Error {};

struct callback_t final {
  constexpr auto operator()(int i) const noexcept -> int { return i + 1; }
};

struct class_verb final {
  struct apply final {
    constexpr auto operator()(fn::some_optional auto &&v, std::invocable<int> auto &&) const noexcept -> decltype(v)
    {
      return std::forward<decltype(v)>(v);
    }
  };
};

union union_verb {
  struct apply final {
    constexpr auto operator()(fn::some_optional auto &&v, std::invocable<int> auto &&) const noexcept -> decltype(v)
    {
      return std::forward<decltype(v)>(v);
    }
  };
};

struct no_apply_verb final {};
} // namespace

TEST_CASE("some_in_place_type", "[traits][some_in_place_type]")
{
  using fn::some_in_place_type;
  using type = std::in_place_type_t<int>;

  static_assert(some_in_place_type<type>);
  static_assert(some_in_place_type<type const>);
  static_assert(some_in_place_type<type &>);
  static_assert(some_in_place_type<type const &>);
  static_assert(some_in_place_type<type &&>);
  static_assert(some_in_place_type<type const &&>);
  static_assert(not some_in_place_type<int>);
  static_assert(not some_in_place_type<std::in_place_t>);
  SUCCEED();
}

TEST_CASE("some_monadic_type", "[traits][some_monadic_type]")
{
  using fn::some_monadic_type;

  static_assert(some_monadic_type<fn::expected<int, bool>>);
  static_assert(some_monadic_type<fn::expected<int, bool> const>);
  static_assert(some_monadic_type<fn::expected<int, bool> &>);
  static_assert(some_monadic_type<fn::expected<int, bool> const &>);
  static_assert(some_monadic_type<fn::expected<int, bool> &&>);
  static_assert(some_monadic_type<fn::expected<int, bool> const &&>);
  static_assert(some_monadic_type<fn::optional<int>>);
  static_assert(some_monadic_type<fn::optional<int> const>);
  static_assert(some_monadic_type<fn::optional<int> &>);
  static_assert(some_monadic_type<fn::optional<int> const &>);
  static_assert(some_monadic_type<fn::optional<int> &&>);
  static_assert(some_monadic_type<fn::optional<int> const &&>);
  SUCCEED();
}

TEST_CASE("monadic_invocable", "[traits][monadic_invocable]")
{
  using fn::monadic_invocable;
  using operand_t = fn::optional<int>;

  SECTION("library verb")
  {
    static_assert(monadic_invocable<fn::transform_t, operand_t, callback_t>);
    static_assert(monadic_invocable<fn::transform_t, fn::expected<int, Error> const &, callback_t>);
    static_assert(not monadic_invocable<fn::transform_t, int, callback_t>);
    static_assert(not monadic_invocable<fn::transform_t, fn::pack<int>, callback_t>);
    SUCCEED();
  }

  SECTION("user-defined verb")
  {
    SECTION("class")
    {
      static_assert(monadic_invocable<class_verb, operand_t, callback_t>);
      static_assert(monadic_invocable<class_verb, operand_t &, callback_t>);
      static_assert(monadic_invocable<class_verb, operand_t const &, callback_t>);
      static_assert(monadic_invocable<class_verb, operand_t &&, callback_t>);
      static_assert(monadic_invocable<class_verb, operand_t const &&, callback_t>);
      static_assert(not monadic_invocable<class_verb, fn::expected<int, Error>, callback_t>);
      static_assert(not monadic_invocable<class_verb, int, callback_t>);
      SUCCEED();
    }

    SECTION("union")
    {
      static_assert(monadic_invocable<union_verb, operand_t, callback_t>);
      static_assert(monadic_invocable<union_verb, operand_t const &, callback_t>);
      static_assert(not monadic_invocable<union_verb, fn::expected<int, Error>, callback_t>);
      static_assert(not monadic_invocable<union_verb, int, callback_t>);
      SUCCEED();
    }

    SECTION("without apply")
    {
      static_assert(not monadic_invocable<no_apply_verb, operand_t, callback_t>);
      SUCCEED();
    }
  }

  SECTION("not a class")
  {
    static_assert(not monadic_invocable<void, operand_t, callback_t>);
    static_assert(not monadic_invocable<int, operand_t, callback_t>);
    static_assert(not monadic_invocable<class_verb &, operand_t, callback_t>);
    SUCCEED();
  }
}
