// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/value_or.hpp>

#include <util/helper_types.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>
#include <utility>

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
      operand_t a{::pfn::unexpect, Error{"Not good"}};
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
      using T = decltype(operand_t{::pfn::unexpect, Error{"Not good"}} | value_or(3));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::pfn::unexpect, Error{"Not good"}} | value_or(3)).value() == 3);
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
        operand_t a{::pfn::unexpect, Error{"Not good"}};
        REQUIRE((std::move(a) | value_or(3, 5)).value().v == 3 * 5);
      }
    }
    WHEN("use move ctor")
    {
      operand_t a{::pfn::unexpect, Error{"Not good"}};
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
  constexpr auto r2 = T{::pfn::unexpect, Error::SomethingElse} | fn::value_or(3);
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
