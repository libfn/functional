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

template <typename... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};

} // namespace

TEST_CASE("inspect with two functions", "[inspect][expected]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  int value = 0;
  std::string error = {};
  auto fnValue = [&value](auto i) -> void { value = i; };
  auto fnError = [&error](auto v) -> void { error = v; };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue),
                                  decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn), decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn1, auto fn2) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](unsigned) -> operand_t { throw 0; },
    [](std::string_view) {})); // allow conversions
  static_assert([](auto &&fn1, auto fn2) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn1),
                             decltype(fn2)>;
  }([](int) -> operand_t { throw 0; }, [](Error) {})); // allow copy from rvalue
  static_assert([](auto &&fn1, auto fn2) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn1),
                             decltype(fn2)>;
  }([](int const &) -> operand_t { throw 0; },
    [](Error const &) {})); // allow binding to const lvalue

  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn1),
                             decltype(fn2)>;
  }([](int) -> operand_t { throw 0; }, [](Error &&) {})); // disallow move
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn1),
                             decltype(fn2)>;
  }([](int &&) -> operand_t { throw 0; }, [](Error) {})); // disallow move
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](int &) -> operand_t { throw 0; },
    [](Error) {})); // disallow removing const-qualifier
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](int) -> operand_t { throw 0; },
    [](Error &) {})); // disallow removing const-qualifier

  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](std::string_view) -> operand_t { throw 0; },
    [](std::string_view) {})); // wrong type
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([]() -> operand_t { throw 0; }, [](std::string_view) {})); // wrong arity
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](auto, auto) -> operand_t { throw 0; },
    [](std::string_view) {})); // wrong arity

  constexpr auto wrong = [](auto) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect(fnValue, wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnValue, wrong)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect(wrong, fnError));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(wrong, fnError))
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
      using T
          = decltype(operand_t{std::in_place, 12} | inspect(fnValue, wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::in_place, 12} | inspect(fnValue, wrong)).value()
              == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | inspect(wrong, fnError));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect(wrong, fnError))
                  .error()
                  .what
              == "Not good");
      CHECK(error == "Not good");
    }
  }
}

TEST_CASE("inspect with one function", "[inspect][expected]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  int value = 0;
  std::string error = {};
  auto fnValue = [&value, &error](auto v) -> void {
    if constexpr (std::same_as<std::decay_t<decltype(v)>, int>) {
      value = v;
    } else {
      error = v;
    }
  };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](int) -> operand_t { throw 0; },
             [](std::string_view) {}})); // allow conversions
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int) -> operand_t { throw 0; },
             [](Error) {}})); // allow copy from rvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int const &) -> operand_t { throw 0; },
             [](Error const &) {}})); // allow binding to const lvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int) -> operand_t { throw 0; },
             [](Error &&) {}})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int &&) -> operand_t { throw 0; },
             [](Error) {}})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](int &) -> operand_t { throw 0; },
             [](Error) {}})); // disallow removing const-qualifier
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](int) -> operand_t { throw 0; },
             [](Error &) {}})); // disallow removing const-qualifier

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](std::string_view) -> operand_t { throw 0; },
             [](std::string_view) {}})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[]() -> operand_t { throw 0; },
             [](std::string_view) {}})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](auto, auto) -> operand_t { throw 0; },
             [](std::string_view) {}})); // wrong arity

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
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(fnValue))
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
      using T = decltype(operand_t{std::in_place, 12} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::in_place, 12} | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect(fnValue))
                  .error()
                  .what
              == "Not good");
      CHECK(error == "Not good");
    }
  }
}

TEST_CASE("inspect with two functions", "[inspect][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  int value = 0;
  int error = 0;
  auto fnValue = [&value](auto i) -> void { value = i; };
  auto fnError = [&error]() -> void { error += 1; };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue),
                                  decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn), decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn1, auto fn2) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](unsigned) -> operand_t { throw 0; }, []() {})); // allow conversions
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](std::string_view) -> operand_t { throw 0; }, []() {})); // wrong type
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([]() -> operand_t { throw 0; }, []() {})); // wrong arity
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](auto) -> operand_t { throw 0; }, [](auto) {})); // wrong arity
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](int &) -> operand_t { throw 0; },
    []() {})); // disallow removing const-qualifier

  constexpr auto wrong = [](auto...) -> void { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | inspect(fnValue, wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnValue, wrong)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | inspect(wrong, fnError));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect(wrong, fnError)).has_value());
      CHECK(error == 1);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | inspect(fnValue, wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{12} | inspect(fnValue, wrong)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | inspect(wrong, fnError));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | inspect(wrong, fnError))
                     .has_value());
      CHECK(error == 1);
    }
  }
}

TEST_CASE("inspect with one function", "[inspect][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  int value = 0;
  int error = 0;
  auto fnValue = [&value, &error](auto... v) -> void {
    if constexpr ((sizeof...(v)) == 1) {
      value = (0 + ... + v);
    } else {
      error += 1;
    }
  };

  static_assert(monadic_invocable<inspect_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](unsigned) -> operand_t { throw 0; },
             []() {}})); // allow conversions
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int) -> operand_t { throw 0; },
             []() {}})); // allow copy from rvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int const &) -> operand_t { throw 0; },
             []() {}})); // allow binding to const lvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t &&, decltype(fn)>;
  }(Overload{[](int &&) -> operand_t { throw 0; }, []() {}})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](int &) -> operand_t { throw 0; },
             []() {}})); // disallow removing const-qualifier

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](std::string_view) -> operand_t { throw 0; },
             []() {}})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([]() -> operand_t { throw 0; })); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](auto, auto) -> operand_t { throw 0; },
             []() {}})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }([](auto) -> operand_t { throw 0; })); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn)>;
  }(Overload{[](auto) -> operand_t { throw 0; },
             [](auto, auto) {}})); // wrong arity

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
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect(fnValue)).has_value());
      CHECK(error == 1);
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
      using T = decltype(operand_t{std::nullopt} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | inspect(fnValue))
                     .has_value());
      CHECK(error == 1);
    }
  }
}
