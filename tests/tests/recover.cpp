// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/recover.hpp"
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

TEST_CASE("recover", "[recover][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto fnError = [](Error e) -> int { return e.what.size(); };

  static_assert(monadic_invocable<recover_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](auto...) -> int { return 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> int { return 0; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](auto...) -> unsigned { return 0; })); // result conversion to operand_t
  static_assert(std::is_same_v<              //
                operand_t,                   //
                decltype(std::declval<operand_t>()
                         | recover([](auto...) -> unsigned { return 0; }))>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](Error &&) -> int { return 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t const, decltype(fn)>;
  }([](Error &&) -> int { return 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t &, decltype(fn)>;
  }([](Error &&) -> int { return 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t &, decltype(fn)>;
  }([](Error &) -> int { return 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t const &, decltype(fn)>;
  }([](Error &) -> int { return 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](Error &) -> int { return 0; })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](int) -> int { return 0; })); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([]() -> int { return 0; })); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](int, int) -> int { return 0; })); // wrong arity

  constexpr auto wrong = [](Error) -> int { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(fnError)).value() == 8);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | recover(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} | recover(fnError)).value()
              == 8);
    }
  }
}

TEST_CASE("recover", "[recover][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  int count = 0;
  auto fnError = [&count](Error) -> void { count += 1; };

  static_assert(monadic_invocable<recover_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](auto...) {})); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](std::string_view) {})); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](Error &&) {})); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t const, decltype(fn)>;
  }([](Error &&) {})); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t &, decltype(fn)>;
  }([](Error &&) {})); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t &, decltype(fn)>;
  }([](Error &) {})); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t const &, decltype(fn)>;
  }([](Error &) {})); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](Error &) {})); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](int) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](Error) {};

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (a | recover(wrong)).value();
      REQUIRE(count == 0);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (a | recover(fnError)).value();
      REQUIRE(count == 1);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | recover(wrong)).value();
      REQUIRE(count == 0);
    }
    WHEN("operand is error")
    {
      using T
          = decltype(operand_t{std::unexpect, "Not good"} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::unexpect, "Not good"} | recover(fnError)).value();
      REQUIRE(count == 1);
    }
  }
}

TEST_CASE("recover", "[recover][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  constexpr auto fnError = []() -> int { return 42; };

  static_assert(monadic_invocable<recover_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](auto...) -> int { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([]() -> unsigned { return 0; })); // result conversion to operand_t
  static_assert(std::is_same_v<       //
                operand_t,            //
                decltype(std::declval<operand_t>()
                         | recover([]() -> unsigned { return 0; }))>);
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<recover_t, operand_t, decltype(fn)>;
  }([](auto) {})); // wrong arity

  constexpr auto wrong = []() -> int { throw 0; };

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(fnError)).value() == 42);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | recover(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::nullopt} | recover(fnError)).value() == 42);
    }
  }
}
