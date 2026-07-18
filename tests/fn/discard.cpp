// Copyright (c) 2025 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/discard.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <utility>

using namespace util;

namespace {
struct Error {
  std::string what;
};

struct DerivedError : Error {};
struct IncompatibleError {};

struct Value {
  int v;
  constexpr bool operator==(Value const &) const = default;
};
} // namespace

TEST_CASE("discard", "[discard][expected][expected_value]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using is = monadic_static_check<discard_t, operand_t>;

  static_assert(is::invocable_with_any());
  static_assert(is::not_invocable_with_any([] {})); // no arguments allowed

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place, 42};
      a | discard();

      REQUIRE(a.value() == 42);
    }

    SECTION("error")
    {
      operand_t a{::fn::unexpect, "Not good"};
      a | discard();

      REQUIRE(a.error().what == "Not good");
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      operand_t{std::in_place, 42} | discard();
      SUCCEED();
    }

    SECTION("error")
    {
      operand_t{::fn::unexpect, "Not good"} | discard();
      SUCCEED();
    }
  }

  SECTION("constexpr")
  {
    enum class Error { ThresholdExceeded, SomethingElse };
    using T = fn::expected<int, Error>;

    constexpr auto a = T{42};
    constexpr auto b = T{::fn::unexpect, Error::ThresholdExceeded};

    constexpr auto test = [](T v) {
      v | discard();
      return true;
    };

    static_assert(test(a));
    static_assert(test(b));

    SUCCEED();
  }
}

TEST_CASE("discard with pack", "[discard][expected][expected_value][pack]")
{
  using namespace fn;

  using operand_t = fn::expected<fn::pack<int, double>, Error>;
  using is = monadic_static_check<discard_t, operand_t>;
  static_assert(is::invocable_with_any());

  SECTION("value")
  {
    constexpr operand_t a{std::in_place, fn::pack{84, 0.5}};
    a | discard();

    REQUIRE(a.has_value());
  }

  SECTION("error")
  {
    operand_t b{::fn::unexpect, "Pack error"};
    b | discard();

    REQUIRE(b.error().what == "Pack error");
  }
}

TEST_CASE("discard", "[discard][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using is = monadic_static_check<discard_t, operand_t>;

  static_assert(is::invocable_with_any());
  static_assert(is::not_invocable_with_any([] {})); // no arguments allowed

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place};
      a | discard();

      REQUIRE(a.has_value());
    }

    SECTION("error")
    {
      operand_t a{::fn::unexpect, "Not good"};
      a | discard();

      REQUIRE(a.error().what == "Not good");
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      operand_t{std::in_place} | discard();
      SUCCEED();
    }

    SECTION("error")
    {
      operand_t{::fn::unexpect, "Not good"} | discard();
      SUCCEED();
    }
  }
}

TEST_CASE("discard", "[discard][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  using is = monadic_static_check<discard_t, operand_t>;

  static_assert(is::invocable_with_any());
  static_assert(is::not_invocable_with_any([] {})); // no arguments allowed

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{42};
      a | discard();

      REQUIRE(a.has_value());
      REQUIRE(a.value() == 42);
    }

    SECTION("nullopt")
    {
      operand_t a{std::nullopt};
      a | discard();

      REQUIRE(!a.has_value());
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      operand_t{42} | discard();
      SUCCEED();
    }

    SECTION("nullopt")
    {
      operand_t{std::nullopt} | discard();
      SUCCEED();
    }
  }

  SECTION("constexpr")
  {
    using T = fn::optional<int>;

    constexpr auto a = T{42};
    constexpr auto b = T{std::nullopt};

    constexpr auto test = [](T v) {
      v | discard();
      return true;
    };

    static_assert(test(a));
    static_assert(test(b));

    SUCCEED();
  }
}

TEST_CASE("discard noexcept", "[discard][noexcept]")
{
  using namespace fn;

  // The one verb whose unconditional noexcept is accurate by construction: discard's apply takes the
  // operand by reference, invokes no callback and returns void, so there is nothing in it that can
  // throw. Asserted positively, where every other verb had to compute a spec.
  static_assert(noexcept(discard_t::apply{}(std::declval<fn::expected<int, Error> &>())));
  static_assert(noexcept(discard_t::apply{}(std::declval<fn::expected<void, Error> &>())));
  static_assert(noexcept(discard_t::apply{}(std::declval<fn::optional<int> &>())));
  static_assert(std::is_same_v<void, decltype(discard_t::apply{}(std::declval<fn::optional<int> &>()))>);

  // and it holds through the pipeline, whatever the operand carries
  static_assert(noexcept(std::declval<fn::expected<int, int> &>() | fn::discard()));
  static_assert(noexcept(std::declval<fn::optional<std::string> &>() | fn::discard()));

  SUCCEED();
}
