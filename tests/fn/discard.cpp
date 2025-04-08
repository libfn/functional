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

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 42};
      a | discard();

      REQUIRE(a.value() == 42);
    }

    WHEN("operand is error")
    {
      operand_t a{std::unexpect, Error{"Not good"}};
      a | discard();

      REQUIRE(a.error().what == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      operand_t{std::in_place, 42} | discard();
      SUCCEED();
    }

    WHEN("operand is error")
    {
      operand_t{std::unexpect, Error{"Not good"}} | discard();
      SUCCEED();
    }
  }
}

TEST_CASE("discard with pack", "[discard][expected][expected_value][pack]")
{
  using namespace fn;

  WHEN("operand is a pack")
  {
    using operand_t = fn::expected<fn::pack<int, double>, Error>;
    constexpr operand_t a{std::in_place, fn::pack{84, 0.5}};

    using is = monadic_static_check<discard_t, operand_t>;
    static_assert(is::invocable_with_any());

    a | discard();

    REQUIRE(a.has_value());

    WHEN("operand is error")
    {
      operand_t b{std::unexpect, Error{"Pack error"}};
      b | discard();

      REQUIRE(b.error().what == "Pack error");
    }
  }
}

TEST_CASE("discard", "[discard][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using is = monadic_static_check<discard_t, operand_t>;

  static_assert(is::invocable_with_any());
  static_assert(is::not_invocable_with_any([] {})); // no arguments allowed

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      a | discard();

      REQUIRE(a.has_value());
    }

    WHEN("operand is error")
    {
      operand_t a{std::unexpect, Error{"Not good"}};
      a | discard();

      REQUIRE(a.error().what == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      operand_t{std::in_place} | discard();
      SUCCEED();
    }

    WHEN("operand is error")
    {
      operand_t{std::unexpect, Error{"Not good"}} | discard();
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

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{42};
      a | discard();

      REQUIRE(a.has_value());
      REQUIRE(a.value() == 42);
    }

    WHEN("operand is nullopt")
    {
      operand_t a{std::nullopt};
      a | discard();

      REQUIRE(!a.has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      operand_t{42} | discard();
      SUCCEED();
    }

    WHEN("operand is nullopt")
    {
      operand_t{std::nullopt} | discard();
      SUCCEED();
    }
  }
}

TEST_CASE("constexpr discard expected", "[discard][constexpr][expected]")
{
  using namespace fn;

  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, Error>;

  constexpr auto a = T{42};
  constexpr auto b = T{std::unexpect, Error::ThresholdExceeded};

  constexpr auto test = [](T v) {
    v | discard();
    return true;
  };

  static_assert(test(a));
  static_assert(test(b));

  SUCCEED();
}

TEST_CASE("constexpr discard optional", "[discard][constexpr][optional]")
{
  using namespace fn;
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
