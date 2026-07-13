// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <util/helper_types.hpp>

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
      operand_t a{::fn::unexpect, Error{"Not good"}};
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
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | recover(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | recover(fnError)).value() == 8);
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
      operand_t a{::fn::unexpect, Error{"Not good"}};
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
  constexpr auto r2 = T{::fn::unexpect, Error::SomethingElse} | fn::recover(fn);
  static_assert(r2.value() == 0);
  constexpr auto r3 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::recover(fn);
  static_assert(r3.value() == 1);

  SUCCEED();
}

TEST_CASE("constexpr recover expected with sum", "[recover][constexpr][expected][sum]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, fn::sum_for<Error, bool>>;

  constexpr auto fn = fn::overload{[](Error e) constexpr noexcept -> int {
                                     if (e == Error::SomethingElse)
                                       return 0;
                                     return 1;
                                   },
                                   [](bool e) constexpr noexcept -> int { return (int)e; }};
  constexpr auto r1 = T{2} | fn::recover(fn);
  static_assert(r1.value() == 2);
  constexpr auto r2 = T{::fn::unexpect, fn::sum{Error::SomethingElse}} | fn::recover(fn);
  static_assert(r2.value() == 0);
  constexpr auto r3 = T{::fn::unexpect, fn::sum{true}} | fn::recover(fn);
  static_assert(r3.value() == 1);
  constexpr auto r4 = T{::fn::unexpect, fn::sum{Error::ThresholdExceeded}} | fn::recover(fn);
  static_assert(r4.value() == 1);

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

TEST_CASE("recover noexcept", "[recover][noexcept]")
{
  // recover invokes AND constructs the result, so it weighs both
  using E = fn::expected<int, int>;
  using O = fn::optional<int>;
  static_assert(noexcept(std::declval<E &>() | fn::recover([](int) noexcept { return 0; })));
  static_assert(not noexcept(std::declval<E &>() | fn::recover([](int) { return 0; })));
  static_assert(noexcept(std::declval<O &>() | fn::recover([]() noexcept { return 0; })));
  static_assert(not noexcept(std::declval<O &>() | fn::recover([]() { return 0; })));

  // constructing the result can throw even where the callback cannot
  using S = fn::expected<std::string, int>;
  static_assert(not noexcept(std::declval<S const &>() | fn::recover([](int) noexcept { return std::string{}; })));
  SUCCEED();
}

TEST_CASE("recover constraints", "[recover][constraints]")
{
  using namespace fn;

  // The success branch carries the existing value over, so it must be able to: an immovable value
  // type must be dropped by the concept, not fail inside the body.
  constexpr auto from_error = [](Error) -> int { return 1; }; // converts to either helper below
  using immovable_t = fn::expected<helper_immovable, Error>;
  static_assert(std::is_constructible_v<immovable_t, std::in_place_t, int>);
  static_assert(monadic_static_check<recover_t, immovable_t>::not_invocable_with_any(from_error));

  // A move-only value type is carried only where it can be moved out of - which is why the question
  // is `is_constructible_v<T, decltype(carried)>` and not `is_move_constructible_v<T>`
  using is = monadic_static_check<recover_t, fn::expected<helper_move_only, Error>>;
  static_assert(is::invocable<rvalue, prvalue>(from_error));     // moved
  static_assert(is::invocable<crvalue, cvalue>(from_error));     // const-moved
  static_assert(is::not_invocable<lvalue, clvalue>(from_error)); // would have to copy

  // A void-valued expected has no value to carry, so nothing constrains it
  static_assert(monadic_static_check<recover_t, fn::expected<void, Error>>::invocable_with_any([](Error) {}));

  // A reference optional binds its referent: the recovered value builds the RESULT, and a reference
  // cannot bind to the prvalue a callback returns
  using ref_t = fn::optional<int &>;
  static_assert(std::is_constructible_v<ref_t::value_type, int>);
  static_assert(not invocable_recover<decltype([]() -> int { return 1; }) &, ref_t &>);
  static_assert(invocable_recover<decltype([]() -> int & {
                                    static int i = 1;
                                    return i;
                                  }) &,
                                  ref_t &>);
  SUCCEED();
}
