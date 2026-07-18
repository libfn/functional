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
  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 12);
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, "Not good"};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 3);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place, 12} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | value_or(3)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, "Not good"} | value_or(3)).value() == 3);
    }
  }

  SECTION("move-only")
  {
    using operand_t = fn::expected<helper_move_only, Error>;
    SECTION("multiple ctor args")
    {
      SECTION("value")
      {
        operand_t a{std::in_place, 2, 3};
        REQUIRE((std::move(a) | value_or(3, 5)).value().v == 2 * 3 * from_rval);
      }
      SECTION("error")
      {
        operand_t a{::fn::unexpect, "Not good"};
        REQUIRE((std::move(a) | value_or(3, 5)).value().v == 3 * 5);
      }
    }
    SECTION("move ctor")
    {
      operand_t a{::fn::unexpect, "Not good"};
      REQUIRE((std::move(a) | value_or(helper_move_only{3, 7})).value().v == 21 * from_rval * from_rval);
    }
  }

  SECTION("constexpr")
  {
    enum class Error { ThresholdExceeded, SomethingElse };
    using T = fn::expected<int, Error>;

    constexpr auto r1 = T{2} | fn::value_or(3);
    static_assert(r1.value() == 2);
    constexpr auto r2 = T{::fn::unexpect, Error::SomethingElse} | fn::value_or(3);
    static_assert(r2.value() == 3);

    SUCCEED();
  }
}

TEST_CASE("value_or", "[value_or][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{12};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 12);
    }
    SECTION("error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | value_or(3)).value() == 3);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{12} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | value_or(3)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{std::nullopt} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::nullopt} | value_or(3)).value() == 3);
    }
  }

  SECTION("move-only")
  {
    using operand_t = fn::optional<helper_move_only>;
    operand_t a{std::nullopt};
    REQUIRE((std::move(a) | value_or(helper_move_only{5, 7})).value().v == 35 * from_rval * from_rval);
  }

  SECTION("constexpr")
  {
    using T = fn::optional<int>;
    constexpr auto r1 = T{0} | fn::value_or(3);
    static_assert(r1.value() == 0);
    constexpr auto r2 = T{} | fn::value_or(3);
    static_assert(r2.value() == 3);

    SUCCEED();
  }
}

TEST_CASE("value_or noexcept", "[value_or][noexcept]")
{
  using namespace fn;

  // value_or takes no callback, but its apply still builds the fallback value from the arguments -
  // inside a lambda it hands to or_else - and that construction can throw: making a std::string from
  // a literal allocates. The apply weighs it.
  static_assert(not std::is_nothrow_constructible_v<std::string, char const *>);
  static_assert(not noexcept(value_or_t::apply{}(std::declval<fn::expected<std::string, Error> &>(), "abc")));
  static_assert(not noexcept(value_or_t::apply{}(std::declval<fn::optional<std::string> &>(), "abc")));

  // ... and nothing else: the error is discarded rather than carried into the result, so its copy
  // never weighs, however throwing it is - Error here holds a std::string
  static_assert(std::is_nothrow_constructible_v<int, int>);
  static_assert(not std::is_nothrow_copy_constructible_v<Error>);
  static_assert(noexcept(value_or_t::apply{}(std::declval<fn::expected<int, Error> &>(), 42)));
  static_assert(noexcept(value_or_t::apply{}(std::declval<fn::expected<int, int> &>(), 42)));
  static_assert(noexcept(value_or_t::apply{}(std::declval<fn::optional<int> &>(), 42)));

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
  static_assert(is::applicable<rvalue, prvalue>(1));    // moved
  static_assert(is::applicable<crvalue, cvalue>(1));    // const-moved
  static_assert(is::not_invocable<lvalue, clvalue>(1)); // would have to copy

  // A reference optional binds its referent rather than carrying it - so an immovable referent is
  // fine, but the fallback builds the RESULT, and a reference cannot bind to a prvalue. The value
  // type alone cannot answer this: it is the referent, which a prvalue constructs happily.
  using ref_t = fn::optional<int &>;
  static_assert(std::is_constructible_v<ref_t::value_type, int>);
  static_assert(not applicable_value_or<ref_t &, int>); // a temporary to bind to: rejected
  static_assert(applicable_value_or<ref_t &, int &>);
  static_assert(applicable_value_or<fn::optional<helper_immovable &> &, helper_immovable &>);
  SUCCEED();
}

namespace fn {
namespace {
struct Error {};
struct Value final {};
} // namespace

// clang-format off
// Not a callable verb: the arguments must construct the operand's own value type.
static_assert(applicable_value_or<expected<int, Error>, int>);
static_assert(applicable_value_or<expected<int, Error>, unsigned>);                 // conversion is enough
static_assert(applicable_value_or<expected<helper_move_only, Error>, int, int>);    // multi-argument construction
static_assert(not applicable_value_or<expected<int, Error>, char const *>);         // no conversion found
static_assert(not applicable_value_or<expected<int, Error>, int, int>);             // too many initialisers
static_assert(not applicable_value_or<expected<Value, Error>, int>);                // wrong type
static_assert(not applicable_value_or<expected<void, Error>, int>);                 // void has no value to fall back to
static_assert(applicable_value_or<optional<int>, int>);
static_assert(not applicable_value_or<optional<int>, char const *>);
static_assert(not applicable_value_or<choice<int>, int>);                           // no choice disjunct
// clang-format on
} // namespace fn
