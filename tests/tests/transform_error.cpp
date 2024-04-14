// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functor.hpp"
#include "functional/transform_error.hpp"

#include <catch2/catch_all.hpp>

#include <string>
#include <utility>

using namespace util;

namespace {
struct Error final {
  std::string what;

  operator std::string_view() const { return what; }
};

struct Xerror final {
  std::size_t value;
};
} // namespace

TEST_CASE("transform_error", "[transform_error][expected]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using is = monadic_static_check<transform_error_t, operand_t>;

  constexpr auto fnError = [](Error v) -> Error { return {"Got: " + v.what}; };
  constexpr auto wrong = [](Error) -> Error { throw 0; };
  constexpr auto fnXerror = [](Error v) -> Xerror { return {v.what.size()}; };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> Error { throw 0; }));                // allow generic call
  static_assert(is::invocable_with_any([](Error) -> Error { throw 0; }));                  // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> Error { throw 0; }));       // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> Error { throw 0; }));          // binds to const ref
  static_assert(is::invocable<lvalue>([](Error &) -> Error { throw 0; }));                 // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](Error &&) -> Error { throw 0; }));       // can move
  static_assert(is::invocable<rvalue, crvalue>([](Error const &&) -> Error { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](Error &) -> Error { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) -> Error { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) -> Error { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> Error { throw 0; }));                       // bad type
  static_assert(is::not_invocable_with_any([]() -> Error { throw 0; }));                                  // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> Error { throw 0; }));                          // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | transform_error(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | transform_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | transform_error(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | transform_error(fnError))
                  .error()
                  .what
              == "Got: Not good");

      WHEN("change type")
      {
        using T = decltype(a | transform_error(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((a | transform_error(fnXerror)).error().value == 8);
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | transform_error(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | transform_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | transform_error(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | transform_error(fnError))
                  .error()
                  .what
              == "Got: Not good");

      WHEN("change type")
      {
        using T = decltype(operand_t{std::unexpect, "Not good"} | transform_error(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((operand_t{std::unexpect, "Not good"} | transform_error(fnXerror)).error().value == 8);
      }
    }
  }
}

TEST_CASE("transform_error", "[transform_error][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  constexpr auto fnError = [](auto...) {};

  // That's all testing needed. Cannot use tranform_error with optional, since
  // there is no error type to operate on
  static_assert(not monadic_invocable<transform_error_t, operand_t, decltype(fnError)>);
}

TEST_CASE("constexpr transform_error expected", "[transform_error][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse, Unknown };
  using T = fn::expected<int, Error>;

  WHEN("same error type")
  {
    constexpr auto fn = [](Error e) constexpr noexcept -> Error {
      if (e == Error::ThresholdExceeded)
        return e;
      return Error::SomethingElse;
    };
    constexpr auto r1 = T{0} | fn::transform_error(fn);
    static_assert(r1.value() == 0);
    constexpr auto r2 = T{std::unexpect, Error::ThresholdExceeded} | fn::transform_error(fn);
    static_assert(r2.error() == Error::ThresholdExceeded);
    constexpr auto r3 = T{std::unexpect, Error::SomethingElse} | fn::transform_error(fn);
    static_assert(r3.error() == Error::SomethingElse);
    constexpr auto r4 = T{std::unexpect, Error::Unknown} | fn::transform_error(fn);
    static_assert(r4.error() == Error::SomethingElse);
  }

  WHEN("different error type")
  {
    struct UnrecoverableError final {
      constexpr UnrecoverableError() {}
      constexpr bool operator==(UnrecoverableError const &) const noexcept = default;
    };
    constexpr auto fn = [](Error) constexpr noexcept -> UnrecoverableError { return {}; };
    constexpr auto r1 = T{0} | fn::transform_error(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<int, UnrecoverableError> const>);
    static_assert(r1.value() == 0);
    constexpr auto r2 = T{std::unexpect, Error::ThresholdExceeded} | fn::transform_error(fn);
    static_assert(r2.error() == UnrecoverableError{});
    constexpr auto r3 = T{std::unexpect, Error::SomethingElse} | fn::transform_error(fn);
    static_assert(r3.error() == UnrecoverableError{});
  }

  SUCCEED();
}

TEST_CASE("constexpr transform_error expected with sum", "[transform_error][constexpr][expected][sum]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, fn::sum<Error, bool>>;

  WHEN("same value type")
  {
    constexpr auto fn = fn::overload{[](bool i) constexpr noexcept -> fn::sum<Error, bool> { return not i; },
                                     [](Error v) constexpr noexcept -> fn::sum<Error, bool> { return v; }};
    constexpr auto r1 = T{std::unexpect, fn::sum{Error::SomethingElse}} | fn::transform_error(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum<Error, bool>> const>);
    static_assert(r1.error() == fn::sum{Error::SomethingElse});
    constexpr auto r2 = T{std::unexpect, fn::sum{true}} | fn::transform_error(fn);
    static_assert(r2.error() == fn::sum{false});
    constexpr auto r3 = T{42} | fn::transform_error(fn);
    static_assert(r3.value() == 42);
  }

  WHEN("different value type")
  {
    constexpr auto fn = fn::overload{[](bool i) constexpr noexcept -> bool { return not i; },
                                     [](Error v) constexpr noexcept -> int { return static_cast<int>(v) + 1; }};
    constexpr auto r1 = T{std::unexpect, fn::sum{Error::SomethingElse}} | fn::transform_error(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum<bool, int>> const>);
    static_assert(r1.error() == fn::sum{2});
    constexpr auto r2 = T{std::unexpect, fn::sum{true}} | fn::transform_error(fn);
    static_assert(r2.error() == fn::sum{false});
    constexpr auto r3 = T{42} | fn::transform_error(fn);
    static_assert(r3.value() == 42);
  }

  SUCCEED();
}
