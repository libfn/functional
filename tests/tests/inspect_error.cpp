// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/inspect_error.hpp"
#include "functional/functor.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

  using operand_t = std::expected<int, Error>;
  std::string error = {};
  auto fnError = [&error](auto v) -> void { error = v; };

  static_assert(
      monadic_invocable<inspect_error_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> void {})); // allow conversions
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](std::string_view) -> int { return 0; })); // wrong return type
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t &&, decltype(fn)>;
  }([](Error) -> void {})); // allow copy from rvalue
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t &&, decltype(fn)>;
  }([](Error const &) -> void {})); // allow binding to const lvalue

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t &&, decltype(fn)>;
  }([](Error &&) -> void {})); // disallow move
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](Error &) -> void {})); // disallow removing const

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](std::in_place_t) -> void {})); // wrong type
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([]() -> void {})); // wrong arity
  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto, auto) -> operand_t { throw 0; })); // wrong arity

  constexpr auto wrong = [](auto) -> void { throw 0; };

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
      REQUIRE((operand_t{std::in_place, 12} | inspect_error(wrong)).value()
              == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | inspect_error(fnError));
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
      using T = decltype(operand_t{std::unexpect, "Not good"}
                         | inspect_error(&Error::finalize));
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

  using operand_t = std::optional<int>;
  int error = 0;
  auto fnError = [&error]() -> void { error += 1; };

  static_assert(
      monadic_invocable<inspect_error_t, operand_t, decltype(fnError)>);
  static_assert([](auto &&fn) constexpr -> bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto...) -> void { throw 0; })); // allow generic call

  static_assert(not [](auto &&fn) constexpr->bool {
    return monadic_invocable<inspect_error_t, operand_t, decltype(fn)>;
  }([](auto) -> void {})); // wrong arity

  constexpr auto wrong = [](auto...) -> void { throw 0; };

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

TEST_CASE("constexpr inspect_error expected",
          "[inspect_error][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = std::expected<int, Error>;
  constexpr auto fn = [](Error) constexpr noexcept -> void {};
  constexpr auto r1 = T{0} | fn::inspect_error(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2
      = T{std::unexpect, Error::SomethingElse} | fn::inspect_error(fn);
  static_assert(r2.error() == Error::SomethingElse);

  SUCCEED();
}

TEST_CASE("constexpr inspect_error optional",
          "[inspect_error][constexpr][optional]")
{
  using T = std::optional<int>;
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

template <typename T>
constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };

constexpr auto fn_int_lvalue = [](int &) {};
constexpr auto fn_int_const_lvalue = [](int const &) {};
constexpr auto fn_int_rvalue = [](int &&) {};
constexpr auto fn_int_const_rvalue = [](int const &&) {};
} // namespace

// clang-format off
static_assert(invocable_inspect_error<decltype(fn_Error<void>), std::expected<int, Error>>);
static_assert(invocable_inspect_error<decltype(fn_generic<void>), std::expected<void, Error>>);
static_assert(invocable_inspect_error<decltype(fn_int<void>), std::expected<void, int>>);
static_assert(not invocable_inspect_error<decltype(fn_int<int>), std::expected<void, int>>); // wrong return type
static_assert(not invocable_inspect_error<decltype(fn_int<void>), std::expected<void, Error>>); // wrong parameter type
static_assert(invocable_inspect_error<decltype(fn_Error<void>), std::expected<void, Xerror>>); // parameter type conversion
static_assert(invocable_inspect_error<decltype(fn_generic<void>), std::expected<Value, Error>>);
static_assert(not invocable_inspect_error<decltype(fn_int<void>), std::expected<Value, Error>>); // wrong parameter type

static_assert(invocable_inspect_error<decltype(fn_generic<void>), std::optional<int>>);
static_assert(not invocable_inspect_error<decltype(fn_generic<int>), std::optional<Value>>); // wrong return type

// binding to const lvalue-ref
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), std::expected<void, int>>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), std::expected<void, int> const>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), std::expected<void, int> &>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), std::expected<void, int> const &>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), std::expected<void, int> &&>);
static_assert(invocable_inspect_error<decltype(fn_int_const_lvalue), std::expected<void, int> const &&>);

// cannot bind const to non-const lvalue-ref
static_assert(not invocable_inspect_error<decltype(fn_int_lvalue), std::expected<void, int>>);
// cannot bind lvalue to const rvalue-ref
static_assert(not invocable_inspect_error<decltype(fn_int_const_rvalue), std::expected<void, int>>);
// cannot bind lvalue to rvalue-ref
static_assert(not invocable_inspect_error<decltype(fn_int_rvalue), std::expected<void, int>>);

// clang-format on
} // namespace fn
