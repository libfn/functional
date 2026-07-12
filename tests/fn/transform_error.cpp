// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/functor.hpp>
#include <fn/transform_error.hpp>

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

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | transform_error(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | transform_error(wrong)).value() == 12);
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | transform_error(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | transform_error(fnError))
                  .error()
                  .what
              == "Got: Not good");

      SECTION("change type")
      {
        using T = decltype(a | transform_error(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((a | transform_error(fnXerror)).error().value == 8);
      }
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place, 12} | transform_error(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | transform_error(wrong)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | transform_error(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
               | transform_error(fnError))
                  .error()
                  .what
              == "Got: Not good");

      SECTION("change type")
      {
        using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | transform_error(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | transform_error(fnXerror)).error().value == 8);
      }
    }
  }

  SECTION("constexpr")
  {
    enum class Error { ThresholdExceeded, SomethingElse, Unknown };
    using T = fn::expected<int, Error>;

    SECTION("same error type")
    {
      constexpr auto fn = [](Error e) constexpr noexcept -> Error {
        if (e == Error::ThresholdExceeded)
          return e;
        return Error::SomethingElse;
      };
      constexpr auto r1 = T{0} | fn::transform_error(fn);
      static_assert(r1.value() == 0);
      constexpr auto r2 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::transform_error(fn);
      static_assert(r2.error() == Error::ThresholdExceeded);
      constexpr auto r3 = T{::fn::unexpect, Error::SomethingElse} | fn::transform_error(fn);
      static_assert(r3.error() == Error::SomethingElse);
      constexpr auto r4 = T{::fn::unexpect, Error::Unknown} | fn::transform_error(fn);
      static_assert(r4.error() == Error::SomethingElse);

      SUCCEED();
    }

    SECTION("different error type")
    {
      struct UnrecoverableError final {
        constexpr UnrecoverableError() {}
        constexpr bool operator==(UnrecoverableError const &) const noexcept = default;
      };
      constexpr auto fn = [](Error) constexpr noexcept -> UnrecoverableError { return {}; };
      constexpr auto r1 = T{0} | fn::transform_error(fn);
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, UnrecoverableError> const>);
      static_assert(r1.value() == 0);
      constexpr auto r2 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::transform_error(fn);
      static_assert(r2.error() == UnrecoverableError{});
      constexpr auto r3 = T{::fn::unexpect, Error::SomethingElse} | fn::transform_error(fn);
      static_assert(r3.error() == UnrecoverableError{});

      SUCCEED();
    }

    SECTION("sum")
    {
      using T = fn::expected<int, fn::sum_for<Error, bool>>;

      SECTION("same error type")
      {
        constexpr auto fn = fn::overload{[](bool i) constexpr noexcept -> fn::sum_for<Error, bool> { return not i; },
                                         [](Error v) constexpr noexcept -> fn::sum_for<Error, bool> { return v; }};
        constexpr auto r1 = T{::fn::unexpect, fn::sum{Error::SomethingElse}} | fn::transform_error(fn);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum_for<Error, bool>> const>);
        static_assert(r1.error() == fn::sum{Error::SomethingElse});
        constexpr auto r2 = T{::fn::unexpect, fn::sum{true}} | fn::transform_error(fn);
        static_assert(r2.error() == fn::sum{false});
        constexpr auto r3 = T{42} | fn::transform_error(fn);
        static_assert(r3.value() == 42);

        SUCCEED();
      }

      SECTION("different error type")
      {
        constexpr auto fn = fn::overload{[](bool i) constexpr noexcept -> bool { return not i; },
                                         [](Error v) constexpr noexcept -> int { return static_cast<int>(v) + 1; }};
        constexpr auto r1 = T{::fn::unexpect, fn::sum{Error::SomethingElse}} | fn::transform_error(fn);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum<bool, int>> const>);
        static_assert(r1.error() == fn::sum{2});
        constexpr auto r2 = T{::fn::unexpect, fn::sum{true}} | fn::transform_error(fn);
        static_assert(r2.error() == fn::sum{false});
        constexpr auto r3 = T{42} | fn::transform_error(fn);
        static_assert(r3.value() == 42);

        SUCCEED();
      }
    }
  }
}

TEST_CASE("transform_error noexcept", "[transform_error][expected][noexcept]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;

  constexpr auto fnNothrow = [](Error const &) noexcept -> Xerror { return {0}; };
  constexpr auto fnThrows = [](Error const &) noexcept(false) -> Xerror { return {0}; };

  // transform_error leaves the VALUE alone, so - mirroring or_else - the member weighs the callback
  // together with the copy of that value. Here the value is an int, whose copy cannot throw, so the
  // callback alone decides.
  static_assert(noexcept(std::declval<operand_t &>().transform_error(fnNothrow)));
  static_assert(not noexcept(std::declval<operand_t &>().transform_error(fnThrows)));

  // Give it a value whose copy can throw, and the member says so even for a noexcept callback - the
  // body really does copy the value across (which is what #278 is about: the requires-clause omits
  // the conjunct that would let a move-only value SFINAE out cleanly, though the noexcept spec below
  // does account for the copy).
  using throwing_value_t = fn::expected<std::string, Error>;
  constexpr auto fnNothrow2 = [](Error const &) noexcept -> Xerror { return {0}; };
  static_assert(not std::is_nothrow_copy_constructible_v<std::string>);
  static_assert(not noexcept(std::declval<throwing_value_t &>().transform_error(fnNothrow2)));

  // GAP #285: transform_error_t::apply discards all of it, being unconditionally noexcept.
  static_assert(noexcept(transform_error_t::apply{}(std::declval<operand_t &>(), fnThrows)));

  SUCCEED();
}

TEST_CASE("transform_error", "[transform_error][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  constexpr auto fnError = [](auto...) {};

  // That's all testing needed. Cannot use tranform_error with optional, since
  // there is no error type to operate on
  static_assert(not monadic_invocable<transform_error_t, operand_t, decltype(fnError)>);

  SUCCEED();
}
