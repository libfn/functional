// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functor.hpp"
#include "functional/recover.hpp"

#include <catch2/catch_all.hpp>

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

  using operand_t = fn::expected<int, Error>;
  using is = static_check<recover_t, operand_t>::bind;

  constexpr auto fnError = [](Error e) -> int { return e.what.size(); };
  constexpr auto wrong = [](Error) -> int { throw 0; };

  static_assert(std::is_same_v<operand_t,
                               decltype(std::declval<operand_t &>() | recover([](auto...) -> unsigned { return 0; }))>);
  static_assert(std::is_same_v<operand_t, decltype(std::declval<operand_t &&>()
                                                   | recover([](auto...) -> unsigned { return 0; }))>);

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> int { throw 0; }));          // allow generic call
  static_assert(is::invocable_with_any([](Error) -> int { throw 0; }));            // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> int { throw 0; })); // allow conversion
  static_assert(is::invocable_with_any([](auto...) -> unsigned { throw 0; }));     // allow conversion to operand_t
  static_assert(is::invocable_with_any([](Error const &) -> int { throw 0; }));    // binds to const ref
  static_assert(is::invocable<lvalue>([](Error &) -> int { throw 0; }));           // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](Error &&) -> int { throw 0; })); // can move
  static_assert(is::invocable<rvalue, crvalue>([](Error const &&) -> int { throw 0; }));       // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](Error &) -> int { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) -> int { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) -> int { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](int) -> int { throw 0; }));                               // bad type
  static_assert(is::not_invocable_with_any([]() -> int { throw 0; }));                                  // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> int { throw 0; }));                          // bad arity

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

  using operand_t = fn::expected<void, Error>;
  using is = static_check<recover_t, operand_t>::bind;

  int count = 0;
  auto fnError = [&count](Error) -> void { count += 1; };
  constexpr auto wrong = [](Error) {};

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) { throw 0; }));                      // allow generic call
  static_assert(is::invocable_with_any([](Error) { throw 0; }));                        // allow copy
  static_assert(is::invocable_with_any([](std::string_view) { throw 0; }));             // allow conversion
  static_assert(is::invocable_with_any([](Error const &) { throw 0; }));                // binds to const ref
  static_assert(is::invocable<lvalue>([](Error &) { throw 0; }));                       // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](Error &&) { throw 0; }));             // can move
  static_assert(is::invocable<rvalue, crvalue>([](Error const &&) { throw 0; }));       // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](Error &) { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) { throw 0; }));                       // bad type
  static_assert(is::not_invocable_with_any([]() { throw 0; }));                                  // bad arity
  static_assert(is::not_invocable_with_any([](int, int) { throw 0; }));                          // bad arity

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

  using operand_t = fn::optional<int>;
  using is = static_check<recover_t, operand_t>::bind;

  constexpr auto fnError = []() -> int { return 42; };
  constexpr auto wrong = []() -> int { throw 0; };

  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t &>() | recover([]() -> unsigned { return 0; }))>);
  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t &&>() | recover([]() -> unsigned { return 0; }))>);

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> int { throw 0; }));      // allow generic call
  static_assert(is::invocable_with_any([]() -> unsigned { throw 0; }));        // allow conversion
  static_assert(is::not_invocable_with_any([](int) -> int { throw 0; }));      // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> int { throw 0; })); // bad arity

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
  using T = fn::expected<int, Error>;

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
  using T = fn::optional<int>;
  constexpr auto fn = []() constexpr noexcept -> int { return 13; };
  constexpr auto r1 = T{0} | fn::recover(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{} | fn::recover(fn);
  static_assert(r2.value() == 13);

  SUCCEED();
}
