// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/fail.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <utility>

namespace {
struct Error {
  std::string what;
};

struct Derived : Error {};
} // namespace

TEST_CASE("fail", "[fail][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto fnValue
      = [](int i) -> Error { return {"Got " + std::to_string(i)}; };

  static_assert(monadic_invocable<fail_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](auto...) -> Error { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](unsigned) -> Error { throw 0; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](auto...) -> Derived { throw 0; })); // result conversion to operand_t
  static_assert(std::is_same_v<            //
                operand_t,                 //
                decltype(std::declval<operand_t>()
                         | fail([](auto...) -> Derived { throw 0; }))>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int &&) -> Error { throw 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t const, decltype(fn)>;
  }([](int &&) -> Error { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t &, decltype(fn)>;
  }([](int &&) -> Error { throw 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t &, decltype(fn)>;
  }([](int &) -> Error { throw 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t const &, decltype(fn)>;
  }([](int &) -> Error { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int &) -> Error { throw 0; })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto fnDerived = [](auto...) -> Derived { return {}; };
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }(fnDerived)); // allow return type conversion
  static_assert(std::is_same_v<operand_t, decltype(std::declval<operand_t>()
                                                   | fail(fnDerived))>);

  constexpr auto wrong = [](int) -> Error { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | fail(fnValue)).error().what == "Got 12");
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | fail(fnValue)).error().what
              == "Got 12");
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("fail", "[fail][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  int count = 0;
  auto fnValue
      = [&count]() -> Error { return {"Got " + std::to_string(++count)}; };

  static_assert(monadic_invocable<fail_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](auto...) -> Error { throw 0; })); // allow generic call
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](auto) -> Error { throw 0; })); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int, int) -> Error { throw 0; })); // wrong arity

  constexpr auto fnDerived = [](auto...) -> Derived { return {}; };
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }(fnDerived)); // allow return type conversion
  static_assert(std::is_same_v<operand_t, decltype(std::declval<operand_t>()
                                                   | fail(fnDerived))>);

  constexpr auto wrong = []() -> Error { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | fail(fnValue)).error().what == "Got 1");
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place} | fail(fnValue)).error().what
              == "Got 1");
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("fail", "[fail][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  auto count = 0;
  auto fnValue = [&count](auto) { count += 1; };

  static_assert(monadic_invocable<fail_t, operand_t, decltype(fnValue)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](auto...) {})); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](unsigned) {})); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int &&) -> void { throw 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t const, decltype(fn)>;
  }([](int &&) -> void { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t &, decltype(fn)>;
  }([](int &&) -> void { throw 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<fail_t, operand_t &, decltype(fn)>;
  }([](int &) -> void { throw 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t const &, decltype(fn)>;
  }([](int &) -> void { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int &) -> void { throw 0; })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<fail_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](auto...) { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | fail(fnValue)).has_value());
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | fail(wrong)).has_value());
      CHECK(count == 0);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::in_place, 12} | fail(fnValue)).has_value());
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | fail(wrong))
                     .has_value());
      CHECK(count == 0);
    }
  }
}
