// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/inspect.hpp"
#include "functional/functor.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {
struct Error final {
  std::string what;

  operator std::string_view() const { return what; }
};

} // namespace

TEST_CASE("inspect expected", "[inspect][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  int value = 0;
  auto fnValue = [&value](auto i) -> void { value = i; };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](unsigned) -> void {})); // allow conversions
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }([](int) -> void {})); // allow copy from rvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }([](int const &) -> void {})); // allow binding to const lvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }([](int &&) -> void {})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](int &) -> void {})); // disallow removing const

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> bool { throw 0; })); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([]() -> void {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto, auto) -> void {})); // wrong arity

  constexpr auto wrong = [](auto) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::in_place, 12} | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("inspect void expected", "[inspect][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  int count = 0;
  auto fnValue = [&count]() -> void { count += 1; };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto) -> void {})); // wrong arity

  constexpr auto wrong = [](auto...) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      (a | inspect(fnValue)).value();
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      (operand_t{std::in_place} | inspect(fnValue)).value();
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("inspect optional", "[inspect][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  int value = 0;
  auto fnValue = [&value](auto i) -> void { value = i; };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](unsigned) -> void {})); // allow conversions
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }([](int) -> void {})); // allow copy from rvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }([](int const &) -> void {})); // allow binding to const lvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }([](int &&) -> void {})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](int &) -> void {})); // disallow removing const

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> void {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([]() -> void {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto, auto) -> void {})); // wrong arity

  constexpr auto wrong = [](auto...) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect(wrong)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{12} | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | inspect(wrong))
                     .has_value());
    }
  }
}
