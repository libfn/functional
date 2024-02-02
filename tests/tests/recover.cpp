// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functor.hpp"
#include "functional/recover.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
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

TEST_CASE("recover", "[recover][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  using is = static_check::bind_right<recover_t>;

  constexpr auto fnError = [](Error e) -> int { return e.what.size(); };

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<recover_t, operand_t, decltype(fnError)>);
  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t>() | recover([](auto...) -> unsigned { return 0; }))>);
  static_assert(is::invocable<operand_t>([](auto...) -> int { return 0; }));          // allow generic call
  static_assert(is::invocable<operand_t>([](std::string_view) -> int { return 0; })); // allow conversion
  static_assert(is::invocable<operand_t>([](auto...) -> unsigned { return 0; }));     // result conversion to operand_t
  static_assert(not is::invocable<operand_t const>([](Error &&) -> int { return 0; }));  // disallow removing const
  static_assert(not is::invocable<operand_t &>([](Error &&) -> int { return 0; }));      // disallow move from lvalue
  static_assert(is::invocable<operand_t &>([](Error &) -> int { return 0; }));           // allow lvalue binding
  static_assert(not is::invocable<operand_t const &>([](Error &) -> int { return 0; })); // disallow removing const
  static_assert(not is::invocable<operand_t>([](int) -> int { return 0; }));             // wrong type
  static_assert(not is::invocable<operand_t>([]() -> int { return 0; }));                // wrong arity
  static_assert(not is::invocable<operand_t>([](int, int) -> int { return 0; }));        // wrong arity

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<recover_t, operand_t &&, decltype(fnError)>);
  static_assert(std::is_same_v<operand_t, decltype(std::declval<operand_t &&>()
                                                   | recover([](auto...) -> unsigned { return 0; }))>);
  static_assert(is::invocable<operand_t &&>([](Error &&) -> int { return 0; }));    // alow move from rvalue
  static_assert(not is::invocable<operand_t &&>([](Error &) -> int { return 0; })); // disallow lvalue binding to rvalue

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
      using T = decltype(operand_t{std::unexpect, "Not good"} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} | recover(fnError)).value() == 8);
    }
  }
}

TEST_CASE("recover", "[recover][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  using is = static_check::bind_right<recover_t>;

  int count = 0;
  auto fnError = [&count](Error) -> void { count += 1; };

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<recover_t, operand_t, decltype(fnError)>);
  static_assert(is::invocable<operand_t>([](auto...) {}));             // allow generic call
  static_assert(is::invocable<operand_t>([](std::string_view) {}));    // allow conversion
  static_assert(not is::invocable<operand_t const>([](Error &&) {}));  // disallow removing const
  static_assert(not is::invocable<operand_t const &>([](Error &) {})); // disallow removing const
  static_assert(not is::invocable<operand_t &>([](Error &&) {}));      // disallow move from lvalue
  static_assert(is::invocable<operand_t &>([](Error &) {}));           // allow lvalue binding
  static_assert(not is::invocable<operand_t>([](int) {}));             // wrong type
  static_assert(not is::invocable<operand_t>([]() {}));                // wrong arity
  static_assert(not is::invocable<operand_t>([](int, int) {}));        // wrong arity

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<recover_t, operand_t &&, decltype(fnError)>);
  static_assert(is::invocable<operand_t &&>([](Error &&) {}));    // alow move from rvalue
  static_assert(not is::invocable<operand_t &&>([](Error &) {})); // disallow lvalue binding to rvalue

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
      using T = decltype(operand_t{std::unexpect, "Not good"} | recover(fnError));
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
  using is = static_check::bind_right<recover_t>;

  constexpr auto fnError = []() -> int { return 42; };

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<recover_t, operand_t, decltype(fnError)>);
  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t>() | recover([]() -> unsigned { return 0; }))>);

  static_assert(is::invocable<operand_t>([](auto...) -> int { throw 0; })); // allow generic call
  static_assert(is::invocable<operand_t>([]() -> unsigned { return 0; }));  // result conversion to operand_t
  static_assert(not is::invocable<operand_t>([](auto) {}));                 // wrong arity

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<recover_t, operand_t &&, decltype(fnError)>);
  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t &&>() | recover([]() -> unsigned { return 0; }))>);

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

TEST_CASE("constexpr recover expected", "[recover][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = std::expected<int, Error>;

  constexpr auto fn = [](Error e) constexpr noexcept -> int {
    if (e == Error::SomethingElse)
      return 0;
    return 1;
  };
  constexpr auto r1 = T{2} | fn::recover(fn);
  static_assert(r1.value() == 2);
  constexpr auto r2 = T{std::unexpect, Error::SomethingElse} | fn::recover(fn);
  static_assert(r2.value() == 0);
  constexpr auto r3 = T{std::unexpect, Error::ThresholdExceeded} | fn::recover(fn);
  static_assert(r3.value() == 1);

  SUCCEED();
}

TEST_CASE("constexpr recover optional", "[recover][constexpr][optional]")
{
  using T = std::optional<int>;
  constexpr auto fn = []() constexpr noexcept -> int { return 13; };
  constexpr auto r1 = T{0} | fn::recover(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{} | fn::recover(fn);
  static_assert(r2.value() == 13);

  SUCCEED();
}
