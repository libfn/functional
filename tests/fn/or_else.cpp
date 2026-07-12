// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/or_else.hpp>

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
  template <typename T> auto fn() noexcept -> T { return {what.size()}; }
  template <typename T> auto finalize() noexcept -> T
  {
    count += static_cast<int>(what.size());
    return {};
  }
};
int Error::count = 0;

struct Xerror final {
  std::string what;
};
} // namespace

TEST_CASE("or_else", "[or_else][expected][expected_value]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using operand_other_t = fn::expected<void, Error>;
  using is = monadic_static_check<or_else_t, operand_t>;

  constexpr auto fnError = [](Error e) -> operand_t { return {e.what.size()}; };
  constexpr auto fnXerror
      = [](Error e) -> fn::expected<int, Xerror> { return ::fn::unexpected<Xerror>{Xerror{"Was: " + e.what}}; };
  constexpr auto wrong = [](Error) -> operand_t { throw 0; };
  constexpr auto fnFail = [](Error v) -> operand_t { return ::fn::unexpected<Error>(Error{"Got: " + v.what}); };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any(fnXerror));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));                // allow generic call
  static_assert(is::invocable_with_any([](Error) -> operand_t { throw 0; }));                  // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> operand_t { throw 0; }));       // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> operand_t { throw 0; }));          // binds to const ref
  static_assert(is::invocable<lvalue>([](Error &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](Error &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::invocable<rvalue, crvalue>([](Error const &&) -> operand_t { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable_with_any([](Error) -> operand_other_t { throw 0; }));        // disallow conversion
  static_assert(
      is::not_invocable<clvalue, crvalue, cvalue>([](Error &) -> operand_t { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) -> operand_t { throw 0; }));         // disallow bind
  static_assert(
      is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) -> operand_t { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> operand_t { throw 0; }));             // bad type
  static_assert(is::not_invocable_with_any([]() -> operand_t { throw 0; }));                        // bad arity
  static_assert(is::not_invocable_with_any([](Error, int) -> operand_t { throw 0; }));              // bad arity

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place, 12};

      SECTION("keep type")
      {
        using T = decltype(a | or_else(wrong));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | or_else(wrong)).value() == 12);
      }

      SECTION("change type")
      {
        using T = decltype(a | or_else(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((a | or_else(wrong)).value() == 12);
      }
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(fnError)).value() == 8);

      SECTION("fail")
      {
        using T = decltype(a | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | or_else(fnFail)).error().what == "Got: Not good");
      }

      SECTION("change error type")
      {
        using T = decltype(a | or_else(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<int, Xerror>>);
        REQUIRE((a | or_else(fnXerror)).error().what == "Was: Not good");
      }
    }
    SECTION("member function")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | or_else(&Error::fn<operand_t>));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(&Error::fn<operand_t>)).value() == 8);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place, 12} | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | or_else(wrong)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | or_else(fnError)).value() == 8);

      SECTION("fail")
      {
        using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
                 | or_else(fnFail))
                    .error()
                    .what
                == "Got: Not good");
      }
    }
    SECTION("member function")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | or_else(&Error::fn<operand_t>));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | or_else(&Error::fn<operand_t>)).value() == 8);
    }
  }

  SECTION("constexpr")
  {
    enum class Error { ThresholdExceeded, SomethingElse };
    using T = fn::expected<int, Error>;

    SECTION("same error type")
    {
      constexpr auto fn = [](Error e) constexpr noexcept -> T {
        if (e == Error::SomethingElse)
          return {0};
        return ::fn::unexpected<Error>{e};
      };
      constexpr auto r1 = T{0} | fn::or_else(fn);
      static_assert(r1.value() == 0);
      constexpr auto r2 = T{::fn::unexpect, Error::SomethingElse} | fn::or_else(fn);
      static_assert(r2.value() == 0);
      constexpr auto r3 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::or_else(fn);
      static_assert(r3.error() == Error::ThresholdExceeded);

      SUCCEED();
    }

    SECTION("different error type")
    {
      struct UnrecoverableError final {
        constexpr UnrecoverableError() {}
        constexpr bool operator==(UnrecoverableError const &) const noexcept = default;
      };
      using T1 = fn::expected<int, UnrecoverableError>;
      constexpr auto fn = [](Error e) constexpr noexcept -> T1 {
        if (e == Error::SomethingElse)
          return {true};
        return T1{::fn::unexpect};
      };
      constexpr auto r1 = T{::fn::unexpect, Error::SomethingElse} | fn::or_else(fn);
      static_assert(std::is_same_v<decltype(r1), fn::expected<int, UnrecoverableError> const>);
      static_assert(r1.value() == true);
      constexpr auto r2 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::or_else(fn);
      static_assert(r2.error() == UnrecoverableError{});

      SUCCEED();
    }

    SECTION("sum")
    {
      enum class Error { ThresholdExceeded, SomethingElse, UnexpectedType };
      using T = fn::expected<int, fn::sum_for<Error, int>>;

      SECTION("same error type")
      {
        constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T {
                                           if (i < 3)
                                             return {i + 1};
                                           return ::fn::unexpected<fn::sum_for<Error, int>>{Error::ThresholdExceeded};
                                         },
                                         [](Error v) constexpr noexcept -> T { return {static_cast<int>(v)}; }};
        constexpr auto r1 = T{::fn::unexpect, 0} | fn::or_else(fn);
        static_assert(r1.value() == 1);
        constexpr auto r2 = T{::fn::unexpect, 3} | fn::or_else(fn);
        static_assert(r2.error() == fn::sum{Error::ThresholdExceeded});

        SUCCEED();
      }

      SECTION("different error type")
      {
        using T1 = fn::expected<int, Error>;
        constexpr auto fn = fn::overload{
            [](int i) constexpr noexcept -> T1 {
              if (i < 2)
                return {i + 1};
              return ::fn::unexpected<Error>{Error::SomethingElse};
            },
            [](Error) constexpr noexcept -> T1 { return ::fn::unexpected<Error>{Error::UnexpectedType}; }};
        constexpr auto r1 = T{::fn::unexpect, 1} | fn::or_else(fn);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, Error> const>);
        static_assert(r1.value() == 2);
        constexpr auto r2 = T{::fn::unexpect, 2} | fn::or_else(fn);
        static_assert(r2.error() == Error::SomethingElse);
        constexpr auto r3 = T{::fn::unexpect, Error::ThresholdExceeded} | fn::or_else(fn);
        static_assert(r3.error() == Error::UnexpectedType);

        SUCCEED();
      }
    }

    SECTION("graded")
    {
      enum class Error : int { Unknown, InvalidValue };
      using T = fn::expected<fn::sum<int>, Error>;

      constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
        if (i == Error::Unknown)
          return {0};
        return ::fn::unexpected<int>{(int)i};
      };

      constexpr auto r1 = T{14} | fn::or_else(fn1);
      static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum<int>, int> const>);
      static_assert(r1.value() == fn::sum{14});
      constexpr auto r2 = T{::fn::unexpect, Error::InvalidValue} | fn::or_else(fn1);
      static_assert(r2.error() == 1);
      constexpr auto r3 = T{::fn::unexpect, Error::Unknown} | fn::or_else(fn1);
      static_assert(r3.value() == fn::sum{0});

      SUCCEED();
    }
  }
}

TEST_CASE("or_else noexcept", "[or_else][expected][expected_value][noexcept]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;

  // Taken by const reference: a by-value Error would copy its std::string on the way in, which is
  // itself a throwing operation and would mask what the callback promises.
  constexpr auto fnNothrow = [](Error const &) noexcept -> operand_t { return {0}; };
  constexpr auto fnThrows = [](Error const &) noexcept(false) -> operand_t { return {0}; };

  // The member's spec is precise: here the untouched value is an int, whose copy cannot throw, so
  // the callback alone decides.
  static_assert(noexcept(std::declval<operand_t &>().or_else(fnNothrow)));
  static_assert(not noexcept(std::declval<operand_t &>().or_else(fnThrows)));

  // It weighs the copy of the untouched VALUE as well - the mirror of what and_then does with the
  // untouched error. Give it a value whose copy can throw and even a noexcept callback leaves the
  // member potentially-throwing.
  using throwing_value_t = fn::expected<std::string, Error>;
  constexpr auto fnNothrow2 = [](Error const &) noexcept -> throwing_value_t { return {""}; };
  static_assert(not std::is_nothrow_copy_constructible_v<std::string>);
  static_assert(not noexcept(std::declval<throwing_value_t &>().or_else(fnNothrow2)));

  // GAP #285: or_else_t::apply then discards all of that, being unconditionally noexcept - as is the
  // rest of the pipeline it is reached through (pinned in tests/fn/functor.cpp).
  static_assert(noexcept(or_else_t::apply{}(std::declval<operand_t &>(), fnThrows)));

  SUCCEED();
}

TEST_CASE("or_else", "[or_else][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using operand_other_t = fn::expected<int, Error>;
  using is = monadic_static_check<or_else_t, operand_t>;

  int count = 0;
  auto fnError = [&count](Error) -> operand_t {
    count += 1;
    return {};
  };
  constexpr auto fnXerror
      = [](Error e) -> fn::expected<void, Xerror> { return ::fn::unexpected<Xerror>{Xerror{"Was: " + e.what}}; };
  constexpr auto wrong = [](Error) -> operand_t { throw 0; };
  constexpr auto fnFail = [](Error v) -> operand_t { return ::fn::unexpected<Error>(Error{"Got: " + v.what}); };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any(fnXerror));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));                // allow generic call
  static_assert(is::invocable_with_any([](Error) -> operand_t { throw 0; }));                  // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> operand_t { throw 0; }));       // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> operand_t { throw 0; }));          // binds to const ref
  static_assert(is::invocable<lvalue>([](Error &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](Error &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::invocable<rvalue, crvalue>([](Error const &&) -> operand_t { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable_with_any([](Error) -> operand_other_t { throw 0; }));        // disallow conversion
  static_assert(
      is::not_invocable<clvalue, crvalue, cvalue>([](Error &) -> operand_t { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) -> operand_t { throw 0; }));         // disallow bind
  static_assert(
      is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) -> operand_t { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> operand_t { throw 0; }));             // bad type
  static_assert(is::not_invocable_with_any([]() -> operand_t { throw 0; }));                        // bad arity
  static_assert(is::not_invocable_with_any([](Error, int) -> operand_t { throw 0; }));              // bad arity

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (a | or_else(wrong)).value();
      CHECK(count == 0);
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (a | or_else(fnError)).value();
      CHECK(count == 1);

      SECTION("fail")
      {
        using T = decltype(a | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | or_else(fnFail)).error().what == "Got: Not good");
        CHECK(count == 1);
      }

      SECTION("change error type")
      {
        using T = decltype(a | or_else(fnXerror));
        static_assert(std::is_same_v<T, fn::expected<void, Xerror>>);
        REQUIRE((a | or_else(fnXerror)).error().what == "Was: Not good");
      }
    }
    SECTION("member function")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | or_else(&Error::finalize<operand_t>));
      static_assert(std::is_same_v<T, operand_t>);
      auto const before = Error::count;
      (a | or_else(&Error::finalize<operand_t>)).value();
      CHECK(Error::count == before + 8);
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place} | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | or_else(wrong)).value();
      CHECK(count == 0);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{::fn::unexpect, Error{"Not good"}} | or_else(fnError)).value();

      SECTION("fail")
      {
        using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
                 | or_else(fnFail))
                    .error()
                    .what
                == "Got: Not good");
      }
    }
    SECTION("member function")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | or_else(&Error::finalize<operand_t>));
      static_assert(std::is_same_v<T, operand_t>);
      auto const before = Error::count;
      (operand_t{::fn::unexpect, Error{"Not good"}} | or_else(&Error::finalize<operand_t>)).value();
      CHECK(Error::count == before + 8);
    }
  }
}

TEST_CASE("or_else noexcept", "[or_else][expected][expected_void][noexcept]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;

  constexpr auto fnNothrow = [](Error const &) noexcept -> operand_t { return {}; };
  constexpr auto fnThrows = [](Error const &) noexcept(false) -> operand_t { return {}; };

  // With a void value there is nothing on the untouched side to copy, so the callback alone decides.
  static_assert(noexcept(std::declval<operand_t &>().or_else(fnNothrow)));
  static_assert(not noexcept(std::declval<operand_t &>().or_else(fnThrows)));

  static_assert(noexcept(or_else_t::apply{}(std::declval<operand_t &>(), fnThrows))); // GAP #285

  SUCCEED();
}

TEST_CASE("or_else", "[or_else][optional]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  using operand_other_t = fn::optional<double>;
  using is = monadic_static_check<or_else_t, operand_t>;

  constexpr auto fnError = []() -> operand_t { return {42}; };
  constexpr auto wrong = []() -> operand_t { throw 0; };
  constexpr auto fnFail = []() -> operand_t { return {}; };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));         // allow generic call
  static_assert(is::not_invocable_with_any([](Error) -> operand_other_t { throw 0; })); // disallow conversion
  static_assert(
      is::not_invocable<clvalue, crvalue, cvalue>([](Error &) -> operand_t { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](Error &) -> operand_t { throw 0; }));         // disallow bind
  static_assert(
      is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](Error &&) -> operand_t { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](int) -> operand_t { throw 0; }));                     // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> operand_t { throw 0; }));                // bad arity

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{12};
      using T = decltype(a | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(wrong)).value() == 12);
    }
    SECTION("error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | or_else(fnError)).value() == 42);

      SECTION("fail")
      {
        using T = decltype(a | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | or_else(fnFail)).has_value());
      }
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{12} | or_else(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | or_else(wrong)).value() == 12);
    }
    SECTION("error")
    {
      using T = decltype(operand_t{std::nullopt} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::nullopt} | or_else(fnError)).value() == 42);

      SECTION("fail")
      {
        using T = decltype(operand_t{std::nullopt} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{std::nullopt} | or_else(fnFail)).has_value());
      }
    }
  }

  SECTION("constexpr")
  {
    using T = fn::optional<int>;
    constexpr auto fn = []() constexpr noexcept -> T { return {1}; };
    constexpr auto r1 = T{0} | fn::or_else(fn);
    static_assert(r1.value() == 0);
    constexpr auto r2 = T{} | fn::or_else(fn);
    static_assert(r2.value() == 1);

    SUCCEED();

    SECTION("sum")
    {
      using T = fn::optional<fn::sum<int>>;
      constexpr auto fn = []() constexpr noexcept -> fn::optional<unsigned long> { return {1ul}; };
      constexpr auto r1 = T{0} | fn::or_else(fn);
      static_assert(std::is_same_v<decltype(r1), fn::optional<fn::sum<int, unsigned long>> const>);
      static_assert(r1.value() == fn::sum{0});
      constexpr auto r2 = T{} | fn::or_else(fn);
      static_assert(r2.value() == fn::sum{1ul});

      SUCCEED();
    }
  }
}

TEST_CASE("or_else noexcept", "[or_else][optional][noexcept]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;

  constexpr auto fnNothrow = []() noexcept -> operand_t { return {42}; };
  constexpr auto fnThrows = []() noexcept(false) -> operand_t { return {42}; };

  static_assert(noexcept(std::declval<operand_t &>().or_else(fnNothrow)));
  static_assert(not noexcept(std::declval<operand_t &>().or_else(fnThrows)));

  static_assert(noexcept(or_else_t::apply{}(std::declval<operand_t &>(), fnThrows))); // GAP #285

  SUCCEED();
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename T> constexpr auto fn_Error = [](Error) -> T { throw 0; };
template <typename T> constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };
template <typename T> constexpr auto fn_int_lvalue = [](int &) -> T { throw 0; };
template <typename T> constexpr auto fn_int_rvalue = [](int &&) -> T { throw 0; };
} // namespace

// clang-format off
static_assert(invocable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<Value, Error>>);
static_assert(invocable_or_else<decltype(fn_generic<expected<int, int>>), expected<int, int>>);
static_assert(invocable_or_else<decltype(fn_Error<expected<Value, Xerror>>), expected<Value, Error>>);   // error type conversion
static_assert(invocable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<Value, Xerror>>);   // error type conversion
static_assert(invocable_or_else<decltype(fn_Error<expected<Value, int>>), expected<Value, Xerror>>);     // error type conversion
static_assert(not invocable_or_else<decltype(fn_Error<expected<Value, int>>), expected<Value, int>>);    // wrong error_type
static_assert(invocable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<Value, Error>>);
static_assert(not invocable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<void, Error>>); // cannot change value_type
static_assert(not invocable_or_else<decltype(fn_Error<expected<void, Error>>), expected<Value, Error>>); // cannot change value_type
static_assert(not invocable_or_else<decltype(fn_generic<expected<Value, int>>), expected<int, int>>);    // cannot change value_type

static_assert(not invocable_or_else<decltype(fn_generic<expected<Value, Error>>), optional<Value>>);     // mixed optional and expected
static_assert(not invocable_or_else<decltype(fn_generic<optional<Value>>), expected<Value, Error>>);     // mixed optional and expected
static_assert(invocable_or_else<decltype(fn_generic<optional<Value>>), optional<Value>>);
static_assert(not invocable_or_else<decltype(fn_generic<optional<int>>), optional<Value>>);              // cannot change value_type
static_assert(not invocable_or_else<decltype(fn_generic<optional<Value>>), optional<int>>);              // cannot change value_type

static_assert(not invocable_or_else<decltype(fn_int_lvalue<expected<int, int>>), expected<int, int>>);   // cannot bind temporary to lvalue
static_assert(invocable_or_else<decltype(fn_int_lvalue<expected<int, int>>), expected<int, int> &>);
static_assert(invocable_or_else<decltype(fn_int_rvalue<expected<int, int>>), expected<int, int>>);
static_assert(not invocable_or_else<decltype(fn_int_rvalue<expected<int, int>>), expected<int, int> &>); // cannot bind lvalue to rvalue-ref
// clang-format on
} // namespace fn
