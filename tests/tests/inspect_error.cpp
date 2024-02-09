// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functor.hpp"
#include "functional/inspect_error.hpp"

#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>
#include <utility>

using namespace util;

namespace {
struct Error final {
  std::string what;
  static int count;

  operator std::string_view() const { return what; }
  auto finalize() const & -> void { count += what.size(); }
};

int Error::count = 0;
} // namespace

TEST_CASE("inspect_error expected", "[inspect_error][expected]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using is = monadic_static_check<inspect_error_t, operand_t>::bind;

  std::string error = {};
  auto fnError = [&error](auto v) -> void { error = v; };
  constexpr auto wrong = [](auto) -> void { throw 0; };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> void { throw 0; }));             // allow generic call
  static_assert(is::invocable_with_any([](Error) -> void { throw 0; }));               // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> void { throw 0; }));    // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> void { throw 0; }));       // binds to const ref
  static_assert(is::not_invocable_with_any([](Error &) -> void { throw 0; }));         // cannot bind lvalue
  static_assert(is::not_invocable_with_any([](Error &&) -> void { throw 0; }));        // cannot move
  static_assert(is::not_invocable_with_any([](std::string_view) -> int { throw 0; })); // bad return type
  static_assert(is::not_invocable_with_any([](std::in_place_t) -> void { throw 0; })); // bad type
  static_assert(is::not_invocable_with_any([]() -> void { throw 0; }));                // bad arity
  static_assert(is::not_invocable_with_any([](Error, int) -> void { throw 0; }));      // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect_error(fnError))
                  .error()
                  .what
              == "Not good");
      CHECK(error == "Not good");
    }
    WHEN("calling member function")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect_error(&Error::finalize));
      static_assert(std::is_same_v<T, operand_t &>);
      auto const before = Error::count;
      REQUIRE((a //
               | inspect_error(&Error::finalize))
                  .error()
                  .what
              == "Not good");
      CHECK(Error::count == before + 8);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::in_place, 12} | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect_error(fnError))
                  .error()
                  .what
              == "Not good");
      CHECK(error == "Not good");
    }
    WHEN("calling member function")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | inspect_error(&Error::finalize));
      static_assert(std::is_same_v<T, operand_t &&>);
      auto const before = Error::count;
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect_error(&Error::finalize))
                  .error()
                  .what
              == "Not good");
      CHECK(Error::count == before + 8);
    }
  }
}

TEST_CASE("inspect_error optional", "[inspect_error][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  using is = monadic_static_check<inspect_error_t, operand_t>::bind;

  int error = 0;
  auto fnError = [&error]() -> void { error += 1; };
  constexpr auto wrong = [](auto...) -> void { throw 0; };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> void { throw 0; }));        // allow generic call
  static_assert(is::not_invocable_with_any([](auto) -> void { throw 0; }));       // bad arity
  static_assert(is::not_invocable_with_any([](auto, auto) -> void { throw 0; })); // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect_error(fnError)).has_value());
      CHECK(error == 1);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | inspect_error(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{12} | inspect_error(wrong)).value() == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | inspect_error(fnError));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | inspect_error(fnError))
                     .has_value());
      CHECK(error == 1);
    }
  }
}

TEST_CASE("constexpr inspect_error expected", "[inspect_error][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, Error>;
  constexpr auto fn = [](Error) constexpr noexcept -> void {};
  constexpr auto r1 = T{0} | fn::inspect_error(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{std::unexpect, Error::SomethingElse} | fn::inspect_error(fn);
  static_assert(r2.error() == Error::SomethingElse);

  SUCCEED();
}

TEST_CASE("constexpr inspect_error optional", "[inspect_error][constexpr][optional]")
{
  using T = fn::optional<int>;
  constexpr auto fn = []() constexpr noexcept -> void {};
  constexpr auto r1 = T{0} | fn::inspect_error(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{} | fn::inspect_error(fn);
  static_assert(not r2.has_value());

  SUCCEED();
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename T> constexpr auto fn_int = [](int) -> T { throw 0; };
template <typename T> constexpr auto fn_Error = [](Error) -> T { throw 0; };
template <typename T> constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };

constexpr auto fn_int_lvalue = [](int &) {};
constexpr auto fn_int_const_lvalue = [](int const &) {};
constexpr auto fn_int_rvalue = [](int &&) {};
constexpr auto fn_int_const_rvalue = [](int const &&) {};
} // namespace

static_assert(invocable_inspect_error<decltype(fn_Error<void>), expected<int, Error>>);
static_assert(invocable_inspect_error<decltype(fn_generic<void>), expected<void, Error>>);
static_assert(invocable_inspect_error<decltype(fn_int<void>), expected<void, int>>);
static_assert(not invocable_inspect_error<decltype(fn_int<int>), expected<void, int>>);    // wrong return type
static_assert(not invocable_inspect_error<decltype(fn_int<void>), expected<void, Error>>); // wrong parameter type
static_assert(invocable_inspect_error<decltype(fn_Error<void>), expected<void, Xerror>>);  // parameter type conversion
static_assert(invocable_inspect_error<decltype(fn_generic<void>), expected<Value, Error>>);
static_assert(not invocable_inspect_error<decltype(fn_int<void>), expected<Value, Error>>); // wrong parameter type
static_assert(invocable_inspect_error<decltype(fn_generic<void>), optional<int>>);
static_assert(not invocable_inspect_error<decltype(fn_generic<int>), optional<Value>>); // wrong return type

// binding to const lvalue-ref
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), expected<void, int>>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), expected<void, int> const>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), expected<void, int> &>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), expected<void, int> const &>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), expected<void, int> &&>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), expected<void, int> const &&>);

// cannot bind const to non-const lvalue-ref
static_assert(not invocable_inspect_error<decltype(fn_int_lvalue), expected<void, int>>);

// cannot bind lvalue to const rvalue-ref
static_assert(not invocable_inspect_error<decltype(fn_int_const_rvalue), expected<void, int>>);

// cannot bind lvalue to rvalue-ref
static_assert(not invocable_inspect_error<decltype(fn_int_rvalue), expected<void, int>>);
} // namespace fn
