// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/transform_error.hpp"
#include "functional/functor.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <utility>

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

  using operand_t = std::expected<int, Error>;
  constexpr auto fnError = [](Error v) -> Error { return {"Got: " + v.what}; };

  static_assert(monadic_invocable<transform_error_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([](auto...) -> Error { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> Error { throw 0; })); // allow conversion
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([](Error &&) -> Error { throw 0; })); // alow move from rvalue
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t const, decltype(fn)>;
  }([](Error &&) -> Error { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t &, decltype(fn)>;
  }([](Error &&) -> Error { throw 0; })); // disallow move from lvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<transform_error_t, operand_t &, decltype(fn)>;
  }([](Error &) -> Error { throw 0; })); // allow lvalue binding
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t const &, decltype(fn)>;
  }([](Error &) -> Error { throw 0; })); // disallow removing const
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([](Error &) -> Error { throw 0; })); // disallow lvalue binding to rvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([](std::string) {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([]() {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<transform_error_t, operand_t, decltype(fn)>;
  }([](int, int) {})); // wrong arity

  constexpr auto wrong = [](Error) -> Error { throw 0; };
  constexpr auto fnXerror = [](Error v) -> Xerror { return {v.what.size()}; };

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
        static_assert(std::is_same_v<T, std::expected<int, Xerror>>);
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
        static_assert(std::is_same_v<T, std::expected<int, Xerror>>);
        REQUIRE((operand_t{std::unexpect, "Not good"} | transform_error(fnXerror)).error().value == 8);
      }
    }
  }
}

TEST_CASE("transform_error", "[transform_error][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  constexpr auto fnError = [](auto...) {};

  // That's all testing needed. Cannot use tranform_error with optional, since
  // there is no error type to operate on
  static_assert(not monadic_invocable<transform_error_t, operand_t, decltype(fnError)>);
}

TEST_CASE("constexpr transform_error expected", "[transform_error][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse, Unknown };
  using T = std::expected<int, Error>;

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
    static_assert(std::is_same_v<decltype(r1), std::expected<int, UnrecoverableError> const>);
    static_assert(r1.value() == 0);
    constexpr auto r2 = T{std::unexpect, Error::ThresholdExceeded} | fn::transform_error(fn);
    static_assert(r2.error() == UnrecoverableError{});
    constexpr auto r3 = T{std::unexpect, Error::SomethingElse} | fn::transform_error(fn);
    static_assert(r3.error() == UnrecoverableError{});
  }

  SUCCEED();
}
