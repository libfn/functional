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

TEST_CASE("inspect", "[inspect][expected]")
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
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](int &) -> operand_t { throw 0; }, [](Error) {})); // non-const
  static_assert(not [](auto &&fn1, auto fn2) constexpr->bool {
    return monadic_invocable<inspect_t, operand_t, decltype(fn1),
                             decltype(fn2)>;
  }([](int) -> operand_t { throw 0; }, [](Error &) {})); // non-const

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

TEST_CASE("inspect", "[inspect][optional]")
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
  }([](int &) -> operand_t { throw 0; }, []() {})); // non-const

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
