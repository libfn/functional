// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/functor.hpp"
#include "functional/inspect.hpp"

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

struct Value final {
  int value;
  static int count;

  bool operator==(Value const &other) const { return value == other.value; }

  auto fn() const & noexcept -> void { count += value; }
};

int Value::count = 0;
} // namespace

TEST_CASE("inspect expected", "[inspect][expected][expected_value][pack]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using is = monadic_static_check<inspect_t, operand_t>;

  int value = 0;
  auto fnValue = [&value](auto i) -> void { value = i; };
  constexpr auto wrong = [](auto) -> void { throw 0; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> void { throw 0; }));         // allow generic call
  static_assert(is::invocable_with_any([](int) -> void { throw 0; }));             // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> void { throw 0; }));        // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> void { throw 0; }));     // binds to const ref
  static_assert(is::not_invocable_with_any([](int &) -> void { throw 0; }));       // cannot bind lvalue
  static_assert(is::not_invocable_with_any([](int &&) -> void { throw 0; }));      // cannot move
  static_assert(is::not_invocable_with_any([](unsigned) -> int { throw 0; }));     // bad return type
  static_assert(is::not_invocable_with_any([](std::string) -> void { throw 0; })); // bad type
  static_assert(is::not_invocable_with_any([]() -> void { throw 0; }));            // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> void { throw 0; }));    // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
    WHEN("calling member function")
    {
      using operand_t = fn::expected<Value, Error>;
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect(&Value::fn));
      static_assert(std::is_same_v<T, operand_t &>);
      auto const before = Value::count;
      REQUIRE((a | inspect(&Value::fn)).value().value == 12);
      CHECK(Value::count == before + 12);
    }
  }

  WHEN("operand is pack")
  {
    WHEN("operand is value")
    {
      using operand_t = fn::expected<fn::pack<int, double>, Error>;
      operand_t a{std::in_place, fn::pack{84, 0.5}};
      int value = 0;
      auto fnPack = [&value](int i, double d) { value = i * d; };
      using T = decltype(a | inspect(fnPack));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnPack)).has_value());
      CHECK(value == 42);
    }

    WHEN("operand is error")
    {
      using operand_t = fn::expected<fn::pack<int, double>, Error>;
      operand_t a{std::unexpect, "Not good"};
      constexpr auto wrong = [](auto...) { throw 0; };
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::in_place, 12} | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
    WHEN("calling member function")
    {
      using operand_t = fn::expected<Value, Error>;
      using T = decltype(operand_t{std::in_place, 12} | inspect(&Value::fn));
      static_assert(std::is_same_v<T, operand_t &&>);
      auto const before = Value::count;
      REQUIRE((operand_t{std::in_place, 12} | inspect(&Value::fn)).value().value == 12);
      CHECK(Value::count == before + 12);
    }
  }
}

TEST_CASE("inspect void expected", "[inspect][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using is = monadic_static_check<inspect_t, operand_t>;

  int count = 0;
  auto fnValue = [&count]() -> void { count += 1; };
  constexpr auto wrong = [](auto...) -> void { throw 0; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> void { throw 0; }));      // allow generic call
  static_assert(is::not_invocable_with_any([]() -> int { throw 0; }));          // bad return type
  static_assert(is::not_invocable_with_any([](int) -> void { throw 0; }));      // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> void { throw 0; })); // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      (a | inspect(fnValue)).value();
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      (operand_t{std::in_place} | inspect(fnValue)).value();
      CHECK(count == 1);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | inspect(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("inspect optional", "[inspect][optional][pack]")
{
  using namespace fn;
  using operand_t = fn::optional<int>;
  using is = monadic_static_check<inspect_t, operand_t>;

  int value = 0;
  auto fnValue = [&value](auto i) -> void { value = i; };
  constexpr auto wrong = [](auto...) -> void { throw 0; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> void { throw 0; }));         // allow generic call
  static_assert(is::invocable_with_any([](int) -> void { throw 0; }));             // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> void { throw 0; }));        // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> void { throw 0; }));     // binds to const ref
  static_assert(is::not_invocable_with_any([](int &) -> void { throw 0; }));       // cannot bind lvalue
  static_assert(is::not_invocable_with_any([](int &&) -> void { throw 0; }));      // cannot move
  static_assert(is::not_invocable_with_any([](unsigned) -> int { throw 0; }));     // bad return type
  static_assert(is::not_invocable_with_any([](std::string) -> void { throw 0; })); // bad type
  static_assert(is::not_invocable_with_any([]() -> void { throw 0; }));            // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> void { throw 0; }));    // bad arity
  static_assert(is::not_invocable_with_any([](int) -> int { return 0; }));         // bad return type

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect(wrong)).has_value());
    }
    WHEN("calling member function")
    {
      using operand_t = fn::optional<Value>;
      operand_t a{std::in_place, 12};
      using T = decltype(a | inspect(&Value::fn));
      static_assert(std::is_same_v<T, operand_t &>);
      auto const before = Value::count;
      REQUIRE((a | inspect(&Value::fn)).value().value == 12);
      CHECK(Value::count == before + 12);
    }
  }

  WHEN("operand is pack")
  {
    WHEN("operand is value")
    {
      using operand_t = fn::optional<fn::pack<int, double>>;
      operand_t a{std::in_place, fn::pack{84, 0.5}};
      int value = 0;
      auto fnPack = [&value](int i, double d) { value = i * d; };
      using T = decltype(a | inspect(fnPack));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE((a | inspect(fnPack)).has_value());
      CHECK(value == 42);
    }

    WHEN("operand is error")
    {
      using operand_t = fn::optional<fn::pack<int, double>>;
      operand_t a{std::nullopt};
      constexpr auto wrong = [](auto...) { throw 0; };
      using T = decltype(a | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &>);
      REQUIRE(not(a | inspect(wrong)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | inspect(fnValue));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE((operand_t{12} | inspect(fnValue)).value() == 12);
      CHECK(value == 12);
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | inspect(wrong));
      static_assert(std::is_same_v<T, operand_t &&>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | inspect(wrong))
                     .has_value());
    }
    WHEN("calling member function")
    {
      using operand_t = fn::optional<Value>;
      using T = decltype(operand_t{std::in_place, 12} | inspect(&Value::fn));
      static_assert(std::is_same_v<T, operand_t &&>);
      auto const before = Value::count;
      REQUIRE((operand_t{std::in_place, 12} | inspect(&Value::fn)).value().value == 12);
      CHECK(Value::count == before + 12);
    }
  }
}

TEST_CASE("inspect choice", "[inspect][choice]")
{
  using namespace fn;

  WHEN("convertible to int")
  {
    using operand_t = fn::choice<bool, int>;
    using is = monadic_static_check<inspect_t, operand_t>;

    static_assert(is::invocable_with_any([](auto) -> void {}));
    static_assert(is::invocable_with_any([](auto...) -> void {}));                // allow generic call
    static_assert(is::invocable_with_any([](int) -> void {}));                    // allow copy
    static_assert(is::invocable_with_any([](unsigned) -> void {}));               // allow conversion
    static_assert(is::invocable_with_any([](int const &) -> void {}));            // binds to const ref
    static_assert(is::not_invocable_with_any([](int &) -> void {}));              // cannot bind lvalue
    static_assert(is::not_invocable_with_any([](int &&) -> void {}));             // cannot move
    static_assert(is::not_invocable_with_any([](unsigned) -> int { return 0; })); // bad return type
    static_assert(is::not_invocable_with_any([](std::string) -> void {}));        // bad type
    static_assert(is::not_invocable_with_any([]() -> void {}));                   // bad arity
    static_assert(is::not_invocable_with_any([](int, int) -> void {}));           // bad arity
    static_assert(is::not_invocable_with_any([](int) -> int { return 0; }));      // bad return type
    constexpr auto fn2 = [](auto const &v) {
      static_assert(std::is_same_v<decltype(v), int const &> || std::is_same_v<decltype(v), bool const &>);
    };
    static_assert(is::invocable_with_any(fn2));

    SUCCEED();
  }

  using operand_t = fn::choice<Value, int>;
  using is = monadic_static_check<inspect_t, operand_t>;

  static_assert(is::invocable_with_any([](auto) {}));
  static_assert(is::invocable_with_any([](auto...) {}));                                         // allow generic call
  static_assert(is::invocable_with_any(fn::overload{[](int) {}, [](Value) {}}));                 // allow copy
  static_assert(is::invocable_with_any(fn::overload{[](unsigned) {}, [](Value) {}}));            // allow conversion
  static_assert(is::invocable_with_any(fn::overload{[](int const &) {}, [](Value const &) {}})); // binds to const ref
  static_assert(is::not_invocable_with_any(fn::overload{[](int &) {}, [](Value &) {}}));         // cannot bind lvalue
  static_assert(is::not_invocable_with_any((fn::overload{[](int &&) {}, [](Value &&) {}})));     // cannot move
  static_assert(is::not_invocable_with_any(
      (fn::overload{[](int const &) -> int { return 0; }, [](Value const &) -> int { return 0; }}))); // bad return type
  constexpr auto fn2 = [](auto const &v) {
    static_assert(std::is_same_v<decltype(v), int const &> || std::is_same_v<decltype(v), Value const &>);
  };
  static_assert(is::invocable_with_any(fn2));

  int value = 0;
  auto fnValue
      = fn::overload([&value](int const &i) { value += i; }, [&value](Value const &i) { value += (i.value / 2); });

  operand_t a{12};
  using T = decltype(a | inspect(fnValue));
  static_assert(std::is_same_v<T, operand_t &>);
  REQUIRE((a | inspect(fnValue)).value() == fn::choice{12});
  CHECK(value == 12);
}

TEST_CASE("constexpr inspect expected", "[inspect][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, Error>;
  constexpr auto fn = [](int) constexpr noexcept -> void {};
  constexpr auto r1 = T{0} | fn::inspect(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{std::unexpect, Error::SomethingElse} | fn::inspect(fn);
  static_assert(r2.error() == Error::SomethingElse);

  SUCCEED();
}

TEST_CASE("constexpr inspect expected with sum", "[inspect][constexpr][expected][sum]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<fn::sum<bool, int>, Error>;
  constexpr auto fn1 = [](int) constexpr noexcept -> void {};
  constexpr auto r11 = T{0} | fn::inspect(fn1);
  static_assert(r11.value() == fn::sum{0});
  constexpr auto fn2 = [](auto const &v) {
    static_assert(std::is_same_v<decltype(v), int const &> || std::is_same_v<decltype(v), bool const &>);
  };
  constexpr auto r12 = T{0} | fn::inspect(fn2);
  static_assert(r12.value() == fn::sum{0});
  constexpr auto r2 = T{std::unexpect, Error::SomethingElse} | fn::inspect(fn1);
  static_assert(r2.error() == Error::SomethingElse);

  SUCCEED();
}

TEST_CASE("constexpr inspect optional", "[inspect][constexpr][optional]")
{
  using T = fn::optional<int>;
  constexpr auto fn = [](int) constexpr noexcept -> void {};
  constexpr auto r1 = T{0} | fn::inspect(fn);
  static_assert(r1.value() == 0);
  constexpr auto r2 = T{} | fn::inspect(fn);
  static_assert(not r2.has_value());

  SUCCEED();
}

TEST_CASE("constexpr inspect optional with sum", "[inspect][constexpr][optional][sum]")
{
  using T = fn::optional<fn::sum<bool, int>>;
  constexpr auto fn1 = [](int) constexpr noexcept -> void {};
  constexpr auto r11 = T{0} | fn::inspect(fn1);
  static_assert(r11.value() == fn::sum{0});
  constexpr auto fn2 = [](auto const &v) {
    static_assert(std::is_same_v<decltype(v), int const &> || std::is_same_v<decltype(v), bool const &>);
  };
  constexpr auto r12 = T{0} | fn::inspect(fn2);
  static_assert(r12.value() == fn::sum{0});
  constexpr auto r2 = T{} | fn::inspect(fn1);
  static_assert(not r2.has_value());

  SUCCEED();
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename T> constexpr auto fn_int = [](int) -> T { throw 0; };
template <typename T> constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };

constexpr auto fn_int_lvalue = [](int &) {};
constexpr auto fn_int_const_lvalue = [](int const &) {};
constexpr auto fn_int_rvalue = [](int &&) {};
constexpr auto fn_int_const_rvalue = [](int const &&) {};
} // namespace

static_assert(invocable_inspect<decltype(fn_int<void>), expected<int, Error>>);
static_assert(not invocable_inspect<decltype(fn_int<int>), expected<int, Error>>); // wrong return type
static_assert(invocable_inspect<decltype(fn_generic<void>), expected<void, Error>>);
static_assert(not invocable_inspect<decltype(fn_generic<int>), expected<void, Error>>); // wrong return type
static_assert(invocable_inspect<decltype(fn_generic<void>), expected<Value, Error>>);
static_assert(not invocable_inspect<decltype(fn_int<void>), expected<Value, Error>>); // wrong parameter type
static_assert(invocable_inspect<decltype(fn_int<void>), expected<unsigned, Xerror>>); // parameter type conversion
static_assert(not invocable_inspect<decltype(fn_int<void>), expected<Value, Error>>); // wrong parameter type
static_assert(invocable_inspect<decltype(fn_generic<void>), optional<int>>);
static_assert(not invocable_inspect<decltype(fn_generic<int>), optional<Value>>); // wrong return type

// binding to const lvalue-ref
static_assert(invocable_inspect<decltype(fn_int_const_lvalue), expected<int, Error>>);
static_assert(invocable_inspect<decltype(fn_int_const_lvalue), expected<int, Error> const>);
static_assert(invocable_inspect<decltype(fn_int_const_lvalue), expected<int, Error> &>);
static_assert(invocable_inspect<decltype(fn_int_const_lvalue), expected<int, Error> const &>);
static_assert(invocable_inspect<decltype(fn_int_const_lvalue), expected<int, Error> &&>);
static_assert(invocable_inspect<decltype(fn_int_const_lvalue), expected<int, Error> const &&>);

// cannot bind const to non-const lvalue-ref
static_assert(not invocable_inspect<decltype(fn_int_lvalue), expected<int, Error>>);

// cannot bind lvalue to const rvalue-ref
static_assert(not invocable_inspect<decltype(fn_int_const_rvalue), expected<int, Error>>);

// cannot bind lvalue to rvalue-ref
static_assert(not invocable_inspect<decltype(fn_int_rvalue), expected<int, Error>>);
} // namespace fn
