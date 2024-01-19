// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/inspect_error.hpp"
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

template <typename... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};

} // namespace

TEST_CASE("inspect_error expected", "[inspect_error][expected]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  std::string error = {};
  auto fnError = [&error](auto v) -> void { error = v; };

  static_assert(
      monadic_invocable<inspect_error_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> void {})); // allow conversions
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t &&, decltype(fn)>;
  }([](Error) -> void {})); // allow copy from rvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t &&, decltype(fn)>;
  }([](Error const &) -> void {})); // allow binding to const lvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t &&, decltype(fn)>;
  }([](Error &&) -> void {})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](Error &) -> void {})); // disallow removing const

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](std::in_place_t) -> void {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([]() -> void {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto, auto) -> operand_t { throw 0; })); // wrong arity

  constexpr auto wrong = [](auto) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect_error(fnError))
                  .error()
                  .what
              == "Not good");
      CHECK(error == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::in_place, 12} | inspect_error(wrong)).value()
              == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect_error(fnError))
                  .error()
                  .what
              == "Not good");
      CHECK(error == "Not good");
    }
  }
}

TEST_CASE("inspect_error optional", "[inspect_error][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  int error = 0;
  auto fnError = [&error]() -> void { error += 1; };

  static_assert(
      monadic_invocable<inspect_error_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto) -> void {})); // wrong arity

  constexpr auto wrong = [](auto...) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect_error(fnError)).has_value());
      CHECK(error == 1);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{12} | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | inspect_error(fnError))
                     .has_value());
      CHECK(error == 1);
    }
  }
}
