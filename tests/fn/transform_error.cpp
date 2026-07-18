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
struct Xerror final {
  std::size_t value;
};

struct Error final {
  std::string what;

  operator std::string_view() const { return what; }
  auto fn() const & -> Xerror { return {what.size()}; }
};

// Instantiating this callable for any argument is a dependent hard error: a verb that compiles
// while receiving it provably never instantiates its callback.
struct Poison final {
  template <typename T> constexpr void operator()(T &&) const { static_assert(sizeof(T) == 0); }
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
  constexpr auto fnVoid = [](Error) {};

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> Error { throw 0; }));                 // allow generic call
  static_assert(is::invocable_with_any([](Error) -> Error { throw 0; }));                   // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> Error { throw 0; }));        // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> Error { throw 0; }));           // binds to const ref
  static_assert(is::applicable<lvalue>([](Error &) -> Error { throw 0; }));                 // binds to lvalue
  static_assert(is::applicable<rvalue, prvalue>([](Error &&) -> Error { throw 0; }));       // can move
  static_assert(is::applicable<rvalue, crvalue>([](Error const &&) -> Error { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](Error &) -> Error { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) -> Error { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) -> Error { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> Error { throw 0; }));                       // bad type
  static_assert(is::not_invocable_with_any([]() -> Error { throw 0; }));                                  // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> Error { throw 0; }));                          // bad arity
  static_assert(is::not_invocable_with_any(fnVoid)); // void return: no unexpected<void> to convert to

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
      operand_t a{::fn::unexpect, "Not good"};
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

      SECTION("member function")
      {
        using T = decltype(a | transform_error(&Error::fn));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((a | transform_error(&Error::fn)).error().value == 8);
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
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | transform_error(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, "Not good"} //
               | transform_error(fnError))
                  .error()
                  .what
              == "Got: Not good");

      SECTION("change type")
      {
        using T = decltype(operand_t{::fn::unexpect, "Not good"} | transform_error(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((operand_t{::fn::unexpect, "Not good"} | transform_error(fnXerror)).error().value == 8);
      }

      SECTION("member function")
      {
        using T = decltype(operand_t{::fn::unexpect, "Not good"} | transform_error(&Error::fn));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((operand_t{::fn::unexpect, "Not good"} | transform_error(&Error::fn)).error().value == 8);
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

    SECTION("copack")
    {
      using T = fn::expected<int, fn::copack_for<Error, bool>>;

      SECTION("same error type")
      {
        constexpr auto fn = fn::overload{[](bool i) constexpr noexcept -> fn::copack_for<Error, bool> { return not i; },
                                         [](Error v) constexpr noexcept -> fn::copack_for<Error, bool> { return v; }};
        constexpr auto r1 = T{::fn::unexpect, fn::copack{Error::SomethingElse}} | fn::transform_error(fn);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::copack_for<Error, bool>> const>);
        static_assert(r1.error() == fn::copack{Error::SomethingElse});
        constexpr auto r2 = T{::fn::unexpect, fn::copack{true}} | fn::transform_error(fn);
        static_assert(r2.error() == fn::copack{false});
        constexpr auto r3 = T{42} | fn::transform_error(fn);
        static_assert(r3.value() == 42);

        SUCCEED();
      }

      SECTION("different error type")
      {
        constexpr auto fn = fn::overload{[](bool i) constexpr noexcept -> bool { return not i; },
                                         [](Error v) constexpr noexcept -> int { return static_cast<int>(v) + 1; }};
        constexpr auto r1 = T{::fn::unexpect, fn::copack{Error::SomethingElse}} | fn::transform_error(fn);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::copack<bool, int>> const>);
        static_assert(r1.error() == fn::copack{2});
        constexpr auto r2 = T{::fn::unexpect, fn::copack{true}} | fn::transform_error(fn);
        static_assert(r2.error() == fn::copack{false});
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
  // body really does copy the value across.
  using throwing_value_t = fn::expected<std::string, Error>;
  constexpr auto fnNothrow2 = [](Error const &) noexcept -> Xerror { return {0}; };
  static_assert(not std::is_nothrow_copy_constructible_v<std::string>);
  static_assert(not noexcept(std::declval<throwing_value_t &>().transform_error(fnNothrow2)));

  // and transform_error_t::apply carries all of it through.
  static_assert(not noexcept(transform_error_t::apply{}(std::declval<operand_t &>(), fnThrows)));

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

TEST_CASE("transform_error identity expected", "[transform_error][expected][copack]")
{
  using operand_t = fn::expected<int, fn::copack<>>;

  // the error side is uninhabited: a dedicated arm delegates to the vacuous member, which accepts
  // any callback and never instantiates it; just and choice stay excluded
  static_assert(monadic_static_check<fn::transform_error_t, operand_t>::invocable_with_any(Poison{}));
  static_assert(monadic_static_check<fn::transform_error_t, fn::just<int>>::not_invocable_with_any(Poison{}));
  static_assert(monadic_static_check<fn::transform_error_t, fn::choice<int>>::not_invocable_with_any(Poison{}));

  operand_t a{5};
  auto r1 = a | fn::transform_error(Poison{});
  static_assert(std::is_same_v<decltype(r1), operand_t>);
  CHECK(r1.value() == 5);
  auto r2 = operand_t{7} | fn::transform_error(Poison{});
  CHECK(r2.value() == 7);
  static_assert((operand_t{5} | fn::transform_error(Poison{})).value() == 5);
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename T> constexpr auto fn_Error = [](Error) -> T { throw 0; };
template <typename T> constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };
constexpr auto fn_Error_lvalue = [](Error &) -> Xerror { throw 0; };
constexpr auto fn_Error_rvalue = [](Error &&) -> Xerror { throw 0; };
} // namespace

// clang-format off
// The callback maps the error; what it returns must convert into an unexpected.
static_assert(applicable_transform_error<decltype(fn_Error<Xerror>), expected<int, Error>>);
static_assert(applicable_transform_error<decltype(fn_Error<Xerror>), expected<void, Error>>);      // void value is fine
static_assert(applicable_transform_error<decltype(fn_generic<Xerror>), expected<Value, Error>>);
static_assert(not applicable_transform_error<decltype(fn_Error<Xerror>), expected<int, Value>>);   // wrong parameter type
static_assert(not applicable_transform_error<decltype(fn_Error<void>), expected<int, Error>>);     // no unexpected<void> to convert to
static_assert(not applicable_transform_error<decltype(fn_generic<Xerror>), optional<int>>);        // optional has no error to map
static_assert(not applicable_transform_error<decltype(fn_generic<Xerror>), choice<int>>);          // neither has choice
static_assert(not applicable_transform_error<decltype(fn_Error_lvalue), expected<int, Error>>);    // cannot bind temporary to lvalue
static_assert(applicable_transform_error<decltype(fn_Error_lvalue), expected<int, Error> &>);
static_assert(applicable_transform_error<decltype(fn_Error_rvalue), expected<int, Error>>);
static_assert(not applicable_transform_error<decltype(fn_Error_rvalue), expected<int, Error> &>);  // cannot bind lvalue to rvalue-ref

// A copack error dispatches through copack::transform - the callback must cover ALL alternatives.
static_assert(applicable_transform_error<decltype(fn_generic<Xerror>), expected<int, copack_for<Error, Value>>>);
static_assert(not applicable_transform_error<decltype(fn_Error<Xerror>), expected<int, copack_for<Error, Value>>>); // not exhaustive
static_assert(not applicable_transform_error<decltype(fn_generic<void>), expected<int, copack_for<Error, Value>>>); // a void result has no place in a copack

// at an uninhabited error side the concept stays false - the dedicated arm, not the concept, admits the operand
static_assert(not applicable_transform_error<decltype(fn_generic<Xerror>), expected<int, copack<>>>);
// clang-format on
} // namespace fn
