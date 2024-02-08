// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/fail.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <utility>

using namespace util;

namespace {
struct Error {
  std::string what;
};

struct Value final {
  int value;
  static int count;

  auto fn() const & -> Error { return {"Was " + std::to_string(value)}; }
  auto finalize() const & -> void { count += value; };
};

int Value::count = 0;

struct Derived : Error {};

} // namespace

TEST_CASE("fail", "[fail][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  using is = static_check<fail_t, operand_t>::bind;

  constexpr auto fnValue = [](int i) -> Error { return {"Got " + std::to_string(i)}; };
  constexpr auto wrong = [](int) -> Error { throw 0; };
  constexpr auto fnDerived = [](auto...) -> Derived { return {}; };

  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t>() | fail([](auto...) -> Derived { throw 0; }))>);
  static_assert(std::is_same_v<operand_t, decltype(std::declval<operand_t>() | fail(fnDerived))>);

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> Error { throw 0; }));                    // allow generic call
  static_assert(is::invocable_with_any([](int) -> Error { throw 0; }));                        // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> Error { throw 0; }));                   // allow conversion
  static_assert(is::invocable_with_any([](auto...) -> Derived { throw 0; }));                  // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> Error { throw 0; }));                // binds to const ref
  static_assert(is::invocable<lvalue>([](int &) -> Error { throw 0; }));                       // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](int &&) -> Error { throw 0; }));             // can move
  static_assert(is::invocable<rvalue, crvalue>([](int const &&) -> Error { throw 0; }));       // binds to const rvalue
  static_assert(is::not_invocable_with_any([](auto...) -> std::string { throw 0; }));          // no conversion found
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](int &) -> Error { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](int &) -> Error { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) -> Error { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](auto...) -> void { throw 0; }));      // bad return type
  static_assert(is::not_invocable_with_any([](std::string) -> Error { throw 0; })); // bad type
  static_assert(is::not_invocable_with_any([]() -> Error { throw 0; }));            // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> Error { throw 0; }));    // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | fail(fnValue)).error().what == "Got 12");
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
    WHEN("calling member function")
    {
      using operand_t = std::expected<Value, Error>;
      operand_t a{std::in_place, 12};
      using T = decltype(a | fail(&Value::fn));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | fail(&Value::fn)).error().what == "Was 12");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | fail(fnValue)).error().what == "Got 12");
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
    WHEN("calling member function")
    {
      using operand_t = std::expected<Value, Error>;
      using T = decltype(operand_t{std::in_place, 12} | fail(&Value::fn));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | fail(&Value::fn)).error().what == "Was 12");
    }
  }
}

TEST_CASE("fail", "[fail][expected][expected_void]")
{
  using namespace fn;

  using operand_t = std::expected<void, Error>;
  using is = static_check<fail_t, operand_t>::bind;

  int count = 0;
  auto fnValue = [&count]() -> Error { return {"Got " + std::to_string(++count)}; };
  constexpr auto fnDerived = [](auto...) -> Derived { return {}; };
  constexpr auto wrong = []() -> Error { throw 0; };

  static_assert(
      std::is_same_v<operand_t, decltype(std::declval<operand_t>() | fail([](auto...) -> Derived { throw 0; }))>);
  static_assert(std::is_same_v<operand_t, decltype(std::declval<operand_t>() | fail(fnDerived))>);

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> Error { throw 0; }));           // allow generic call
  static_assert(is::invocable_with_any([](auto...) -> Derived { throw 0; }));         // allow conversion
  static_assert(is::not_invocable_with_any([](auto...) -> std::string { throw 0; })); // no conversion found
  static_assert(is::not_invocable_with_any([](auto...) -> void { throw 0; }));        // bad return type
  static_assert(is::not_invocable_with_any([](std::string) -> Error { throw 0; }));   // bad type
  static_assert(is::not_invocable_with_any([](int) -> Error { throw 0; }));           // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> Error { throw 0; }));      // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | fail(fnValue)).error().what == "Got 1");
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place} | fail(fnValue)).error().what == "Got 1");
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | fail(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("fail", "[fail][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  using is = static_check<fail_t, operand_t>::bind;

  auto count = 0;
  auto fnValue = [&count](auto) { count += 1; };
  constexpr auto wrong = [](auto...) { throw 0; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) { throw 0; }));                             // allow generic call
  static_assert(is::invocable_with_any([](int) { throw 0; }));                                 // allow copy
  static_assert(is::invocable_with_any([](unsigned) { throw 0; }));                            // allow conversion
  static_assert(is::invocable_with_any([](int const &) { throw 0; }));                         // binds to const ref
  static_assert(is::invocable<lvalue>([](int &) { throw 0; }));                                // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](int &&) { throw 0; }));                      // can move
  static_assert(is::invocable<rvalue, crvalue>([](int const &&) { throw 0; }));                // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](int &) { throw 0; }));          // cannot remove const
  static_assert(is::not_invocable<rvalue>([](int &) { throw 0; }));                            // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](auto...) -> Error { throw 0; }));                // wrong return type
  static_assert(is::not_invocable_with_any([](std::string) { throw 0; }));                     // bad type
  static_assert(is::not_invocable_with_any([]() { throw 0; }));                                // bad arity
  static_assert(is::not_invocable_with_any([](int, int) { throw 0; }));                        // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | fail(fnValue)).has_value());
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | fail(wrong)).has_value());
      CHECK(count == 0);
    }
    WHEN("calling member function")
    {
      using operand_t = std::optional<Value>;
      operand_t a{std::in_place, 12};
      using T = decltype(a | fail(&Value::finalize));
      static_assert(std::is_same_v<T, operand_t>);
      auto const before = Value::count;
      REQUIRE(not(a | fail(&Value::finalize)).has_value());
      CHECK(Value::count == before + 12);
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | fail(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::in_place, 12} | fail(fnValue)).has_value());
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | fail(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | fail(wrong))
                     .has_value());
      CHECK(count == 0);
    }
    WHEN("calling member function")
    {
      using operand_t = std::optional<Value>;
      using T = decltype(operand_t{std::in_place, 12} | fail(&Value::finalize));
      static_assert(std::is_same_v<T, operand_t>);
      auto const before = Value::count;
      REQUIRE(not(operand_t{std::in_place, 12} | fail(&Value::finalize)).has_value());
      CHECK(Value::count == before + 12);
    }
  }
}

TEST_CASE("constexpr fail expected", "[fail][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = std::expected<int, Error>;
  constexpr auto fn = [](int i) constexpr noexcept -> Error {
    if (i < 3)
      return Error::SomethingElse;
    return Error::ThresholdExceeded;
  };
  constexpr auto r1 = T{0} | fn::fail(fn);
  static_assert(r1.error() == Error::SomethingElse);
  constexpr auto r2 = T{3} | fn::fail(fn);
  static_assert(r2.error() == Error::ThresholdExceeded);

  SUCCEED();
}

TEST_CASE("constexpr fail optional", "[fail][constexpr][optional]")
{
  using T = std::optional<int>;
  constexpr auto fn = [](int) constexpr noexcept -> void {};
  constexpr auto r1 = T{0} | fn::fail(fn);
  static_assert(not r1.has_value());

  SUCCEED();
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename E> constexpr auto fn_int = [](int) -> E { throw 0; };
template <typename E> constexpr auto fn_generic = [](auto &&...) -> E { throw 0; };
template <typename E> constexpr auto fn_int_lvalue = [](int &) -> E { throw 0; };
template <typename E> constexpr auto fn_int_rvalue = [](int &&) -> E { throw 0; };
} // namespace

static_assert(invocable_fail<decltype(fn_int<Error>), std::expected<int, Error>>);
static_assert(invocable_fail<decltype(fn_generic<Error>), std::expected<void, Error>>);
static_assert(invocable_fail<decltype(fn_int<Xerror>), std::expected<int, Error>>);           // return type conversion
static_assert(not invocable_fail<decltype(fn_int<Error>), std::expected<int, Xerror>>);       // cannot convert error
static_assert(not invocable_fail<decltype(fn_generic<Error>), std::expected<void, Xerror>>);  // cannot convert error
static_assert(not invocable_fail<decltype(fn_generic<Error>), std::expected<Value, Xerror>>); // cannot convert error
static_assert(not invocable_fail<decltype(fn_int<Error>), std::expected<Value, Error>>);      // wrong parameter type
static_assert(invocable_fail<decltype(fn_generic<Error>), std::expected<Value, Error>>);
static_assert(invocable_fail<decltype(fn_int<void>), std::optional<int>>);
static_assert(not invocable_fail<decltype(fn_int<void>), std::optional<Value>>); // wrong parameter type
static_assert(invocable_fail<decltype(fn_generic<void>), std::optional<Value>>);
static_assert(not invocable_fail<decltype(fn_generic<Error>), std::optional<Value>>); // bad return type
static_assert(
    not invocable_fail<decltype(fn_int_lvalue<Error>), std::expected<int, Error>>); // cannot bind temporary to lvalue
static_assert(invocable_fail<decltype(fn_int_lvalue<Error>), std::expected<int, Error> &>);
static_assert(invocable_fail<decltype(fn_int_rvalue<Error>), std::expected<int, Error>>);
static_assert(not invocable_fail<decltype(fn_int_rvalue<Error>),
                                 std::expected<int, Error> &>); // cannot bind lvalue to rvalue-ref
} // namespace fn
