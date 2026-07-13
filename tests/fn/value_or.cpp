// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/value_or.hpp>

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>
#include <utility>

using namespace util;

namespace {
struct Error final {
  std::string what;

  operator std::string_view() const { return what; }
};
} // namespace

TEST_CASE("value_or", "[value_or][expected][expected_value]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 3);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | value_or(3)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | value_or(3)).value() == 3);
    }
  }

  WHEN("move only argument")
  {
    using operand_t = fn::expected<helper_move_only, Error>;
    WHEN("pass multiple constructor arguments")
    {
      WHEN("operand is value")
      {
        operand_t a{std::in_place, 2, 3};
        REQUIRE((std::move(a) | value_or(3, 5)).value().v == 2 * 3 * from_rval);
      }
      WHEN("operand is error")
      {
        operand_t a{::fn::unexpect, Error{"Not good"}};
        REQUIRE((std::move(a) | value_or(3, 5)).value().v == 3 * 5);
      }
    }
    WHEN("use move ctor")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      REQUIRE((std::move(a) | value_or(helper_move_only{3, 7})).value().v == 21 * from_rval * from_rval);
    }
  }
}

TEST_CASE("value_or", "[value_or][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 3);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | value_or(3)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::nullopt} | value_or(3)).value() == 3);
    }
  }

  WHEN("move only argument")
  {
    using operand_t = fn::optional<helper_move_only>;
    operand_t a{std::nullopt};
    REQUIRE((std::move(a) | value_or(helper_move_only{5, 7})).value().v == 35 * from_rval * from_rval);
  }
}

TEST_CASE("constexpr value_or expected", "[value_or][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, Error>;

  constexpr auto r1 = T{2} | fn::value_or(3);
  static_assert(r1.value() == 2);
  constexpr auto r2 = T{::fn::unexpect, Error::SomethingElse} | fn::value_or(3);
  static_assert(r2.value() == 3);

  SUCCEED();
}

TEST_CASE("constexpr value_or optional", "[value_or][constexpr][optional]")
{
  using T = fn::optional<int>;
  constexpr auto r1 = T{0} | fn::value_or(3);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{} | fn::value_or(3);
  static_assert(r2.value() == 3);

  SUCCEED();
}

TEST_CASE("value_or noexcept", "[value_or][noexcept]")
{
  using E = fn::expected<int, int>;
  using O = fn::optional<int>;
  static_assert(noexcept(std::declval<E &>() | fn::value_or(1)));
  static_assert(noexcept(std::declval<O &>() | fn::value_or(1)));

  // building the fallback value can throw
  using S = fn::optional<std::string>;
  static_assert(not noexcept(std::declval<S &>() | fn::value_or("x")));

  // the untouched error is never relocated, so its throwing copy does not weigh
  using X = fn::expected<int, std::string>;
  static_assert(noexcept(std::declval<X &>() | fn::value_or(1)));

  // the carried value is relocated in the operand's category, and that weighs
  struct MoveNothrow {
    MoveNothrow(int) noexcept {}
    MoveNothrow(MoveNothrow const &) noexcept(false) {}
    MoveNothrow(MoveNothrow &&) noexcept {}
  };
  using W = fn::expected<MoveNothrow, int>;
  static_assert(not noexcept(std::declval<W &>() | fn::value_or(1))); // copies
  static_assert(noexcept(std::declval<W &&>() | fn::value_or(1)));    // moves
  SUCCEED();
}

TEST_CASE("value_or constraints", "[value_or][constraints]")
{
  using namespace fn;

  // The result carries the existing value over, so it must be able to. An immovable value type can
  // still be built in place - which is not the question - but never carried: the candidate must
  // drop, not fail inside the body.
  using immovable_t = fn::expected<helper_immovable, Error>;
  static_assert(std::is_constructible_v<immovable_t, std::in_place_t, int>);
  static_assert(monadic_static_check<value_or_t, immovable_t>::not_invocable_with_any(1));
  static_assert(monadic_static_check<value_or_t, fn::optional<helper_immovable>>::not_invocable_with_any(1));

  // A move-only value type is carried only where it can be moved out of. This is what makes the
  // question `is_constructible_v<T, decltype(carried)>` and not `is_move_constructible_v<T>` - the
  // latter would accept every category below, including the two that would have to copy.
  using move_only_t = fn::expected<helper_move_only, Error>;
  using is = monadic_static_check<value_or_t, move_only_t>;
  static_assert(is::invocable<rvalue, prvalue>(1));     // moved
  static_assert(is::invocable<crvalue, cvalue>(1));     // const-moved
  static_assert(is::not_invocable<lvalue, clvalue>(1)); // would have to copy

  // A reference optional binds its referent rather than carrying it - so an immovable referent is
  // fine, but the fallback builds the RESULT, and a reference cannot bind to a prvalue. The value
  // type alone cannot answer this: it is the referent, which a prvalue constructs happily.
  using ref_t = fn::optional<int &>;
  static_assert(std::is_constructible_v<ref_t::value_type, int>);
  static_assert(not invocable_value_or<ref_t &, int>); // a temporary to bind to: rejected
  static_assert(invocable_value_or<ref_t &, int &>);
  static_assert(invocable_value_or<fn::optional<helper_immovable &> &, helper_immovable &>);
  SUCCEED();
}
