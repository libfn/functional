// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/functor.hpp>
#include <fn/recover.hpp>

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
  using is = monadic_static_check<recover_t, operand_t>;

  constexpr auto fnError = [](Error e) -> int { return static_cast<int>(e.what.size()); };
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

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(wrong)).value() == 12);
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(fnError)).value() == 8);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place, 12} | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | recover(wrong)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | recover(fnError)).value() == 8);
    }
  }

  SECTION("constexpr")
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
    constexpr auto r2 = T{::fn::unexpect, Error::SomethingElse} | fn::recover(fn);
    static_assert(r2.value() == 0);
    constexpr auto r3 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::recover(fn);
    static_assert(r3.value() == 1);

    SUCCEED();

    SECTION("sum")
    {
      using TS = fn::expected<int, fn::sum_for<Error, bool>>;

      constexpr auto fnSum = fn::overload{[](Error e) constexpr noexcept -> int {
                                            if (e == Error::SomethingElse)
                                              return 0;
                                            return 1;
                                          },
                                          [](bool e) constexpr noexcept -> int { return (int)e; }};
      constexpr auto s1 = TS{2} | fn::recover(fnSum);
      static_assert(s1.value() == 2);
      constexpr auto s2 = TS{::fn::unexpect, fn::sum{Error::SomethingElse}} | fn::recover(fnSum);
      static_assert(s2.value() == 0);
      constexpr auto s3 = TS{::fn::unexpect, fn::sum{true}} | fn::recover(fnSum);
      static_assert(s3.value() == 1);
      constexpr auto s4 = TS{::fn::unexpect, fn::sum{Error::ThresholdExceeded}} | fn::recover(fnSum);
      static_assert(s4.value() == 1);

      SUCCEED();
    }
  }
}

TEST_CASE("recover", "[recover][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using is = monadic_static_check<recover_t, operand_t>;

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

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (a | recover(wrong)).value();
      REQUIRE(count == 0);
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (a | recover(fnError)).value();
      REQUIRE(count == 1);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place} | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | recover(wrong)).value();
      REQUIRE(count == 0);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{::fn::unexpect, Error{"Not good"}} | recover(fnError)).value();
      REQUIRE(count == 1);
    }
  }
}

TEST_CASE("recover", "[recover][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  using is = monadic_static_check<recover_t, operand_t>;

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

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{12};
      using T = decltype(a | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(wrong)).value() == 12);
    }
    SECTION("error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | recover(fnError)).value() == 42);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{12} | recover(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | recover(wrong)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{std::nullopt} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::nullopt} | recover(fnError)).value() == 42);
    }
  }

  SECTION("constexpr")
  {
    using T = fn::optional<int>;
    constexpr auto fn = []() constexpr noexcept -> int { return 13; };
    constexpr auto r1 = T{0} | fn::recover(fn);
    static_assert(r1.value() == 0);
    constexpr auto r2 = T{} | fn::recover(fn);
    static_assert(r2.value() == 13);

    SUCCEED();
  }
}

TEST_CASE("recover noexcept", "[recover][noexcept]")
{
  using namespace fn;

  constexpr auto fnThrows = [](Error const &) noexcept(false) -> int { return 0; };
  constexpr auto fnThrows0 = [](Error const &) noexcept(false) -> void {};
  constexpr auto fnThrowsOpt = []() noexcept(false) -> int { return 0; };

  // GAP #285: recover's apply is the implementation too - it invokes the callback AND constructs the
  // recovered result from what comes back, both under an unconditional noexcept. So it has two ways
  // to throw and promises neither, with no member spec anywhere to appeal to.
  static_assert(noexcept(recover_t::apply{}(std::declval<fn::expected<int, Error> &>(), fnThrows)));
  static_assert(noexcept(recover_t::apply{}(std::declval<fn::expected<void, Error> &>(), fnThrows0)));
  static_assert(noexcept(recover_t::apply{}(std::declval<fn::optional<int> &>(), fnThrowsOpt)));

  SUCCEED();
}
