// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/just.hpp>
#include <fn/or_else.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <fn/detail/macro_begin.hpp>

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

// Instantiating this callable for any argument is a dependent hard error: a verb that compiles
// while receiving it provably never instantiates its callback.
struct Poison final {
  template <typename T> constexpr void operator()(T &&) const { static_assert(sizeof(T) == 0); }
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
      = [](Error e) -> fn::expected<int, Xerror> { return ::fn::unexpected<Xerror>{"Was: " + e.what}; };
  constexpr auto wrong = [](Error) -> operand_t { throw 0; };
  constexpr auto fnFail = [](Error v) -> operand_t { return ::fn::unexpected<Error>("Got: " + v.what); };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any(fnXerror));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));                 // allow generic call
  static_assert(is::invocable_with_any([](Error) -> operand_t { throw 0; }));                   // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> operand_t { throw 0; }));        // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> operand_t { throw 0; }));           // binds to const ref
  static_assert(is::applicable<lvalue>([](Error &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::applicable<rvalue, prvalue>([](Error &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::applicable<rvalue, crvalue>([](Error const &&) -> operand_t { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable_with_any([](Error) -> operand_other_t { throw 0; }));         // disallow conversion
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
      operand_t a{::fn::unexpect, "Not good"};
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
      operand_t a{::fn::unexpect, "Not good"};
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
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, "Not good"} | or_else(fnError)).value() == 8);

      SECTION("fail")
      {
        using T = decltype(operand_t{::fn::unexpect, "Not good"} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{::fn::unexpect, "Not good"} //
                 | or_else(fnFail))
                    .error()
                    .what
                == "Got: Not good");
      }
    }
    SECTION("member function")
    {
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | or_else(&Error::fn<operand_t>));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, "Not good"} | or_else(&Error::fn<operand_t>)).value() == 8);
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

    SECTION("copack")
    {
      enum class Error { ThresholdExceeded, SomethingElse, UnexpectedType };
      using T = fn::expected<int, fn::copack_for<Error, int>>;

      SECTION("same error type")
      {
        constexpr auto fn
            = fn::overload{[](int i) constexpr noexcept -> T {
                             if (i < 3)
                               return {i + 1};
                             return ::fn::unexpected<fn::copack_for<Error, int>>{Error::ThresholdExceeded};
                           },
                           [](Error v) constexpr noexcept -> T { return {static_cast<int>(v)}; }};
        constexpr auto r1 = T{::fn::unexpect, 0} | fn::or_else(fn);
        static_assert(r1.value() == 1);
        constexpr auto r2 = T{::fn::unexpect, 3} | fn::or_else(fn);
        static_assert(r2.error() == fn::copack{Error::ThresholdExceeded});

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
      using T = fn::expected<fn::copack<int>, Error>;

      constexpr auto fn1 = [](Error i) -> fn::expected<int, int> {
        if (i == Error::Unknown)
          return {0};
        return ::fn::unexpected<int>{(int)i};
      };

      constexpr auto r1 = T{14} | fn::or_else(fn1);
      static_assert(std::is_same_v<decltype(r1), fn::expected<fn::copack<int>, int> const>);
      static_assert(r1.value() == fn::copack{14});
      constexpr auto r2 = T{::fn::unexpect, Error::InvalidValue} | fn::or_else(fn1);
      static_assert(r2.error() == 1);
      constexpr auto r3 = T{::fn::unexpect, Error::Unknown} | fn::or_else(fn1);
      static_assert(r3.value() == fn::copack{0});

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

  // and or_else_t::apply carries all of that through, as does the rest of the pipeline it is
  // reached through (pinned in tests/fn/functor.cpp).
  static_assert(not noexcept(or_else_t::apply{}(std::declval<operand_t &>(), fnThrows)));

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
      = [](Error e) -> fn::expected<void, Xerror> { return ::fn::unexpected<Xerror>{"Was: " + e.what}; };
  constexpr auto wrong = [](Error) -> operand_t { throw 0; };
  constexpr auto fnFail = [](Error v) -> operand_t { return ::fn::unexpected<Error>("Got: " + v.what); };

  static_assert(is::invocable_with_any(fnError));
  static_assert(is::invocable_with_any(fnXerror));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));                 // allow generic call
  static_assert(is::invocable_with_any([](Error) -> operand_t { throw 0; }));                   // allow copy
  static_assert(is::invocable_with_any([](std::string_view) -> operand_t { throw 0; }));        // allow conversion
  static_assert(is::invocable_with_any([](Error const &) -> operand_t { throw 0; }));           // binds to const ref
  static_assert(is::applicable<lvalue>([](Error &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::applicable<rvalue, prvalue>([](Error &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::applicable<rvalue, crvalue>([](Error const &&) -> operand_t { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable_with_any([](Error) -> operand_other_t { throw 0; }));         // disallow conversion
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
      operand_t a{::fn::unexpect, "Not good"};
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
      operand_t a{::fn::unexpect, "Not good"};
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
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | or_else(fnError));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{::fn::unexpect, "Not good"} | or_else(fnError)).value();

      SECTION("fail")
      {
        using T = decltype(operand_t{::fn::unexpect, "Not good"} | or_else(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{::fn::unexpect, "Not good"} //
                 | or_else(fnFail))
                    .error()
                    .what
                == "Got: Not good");
      }
    }
    SECTION("member function")
    {
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | or_else(&Error::finalize<operand_t>));
      static_assert(std::is_same_v<T, operand_t>);
      auto const before = Error::count;
      (operand_t{::fn::unexpect, "Not good"} | or_else(&Error::finalize<operand_t>)).value();
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

  static_assert(not noexcept(or_else_t::apply{}(std::declval<operand_t &>(), fnThrows)));

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

    SECTION("copack")
    {
      using T = fn::optional<fn::copack<int>>;
      constexpr auto fn = []() constexpr noexcept -> fn::optional<unsigned long> { return {1ul}; };
      constexpr auto r1 = T{0} | fn::or_else(fn);
      static_assert(std::is_same_v<decltype(r1), fn::optional<fn::copack<int, unsigned long>> const>);
      static_assert(r1.value() == fn::copack{0});
      constexpr auto r2 = T{} | fn::or_else(fn);
      static_assert(r2.value() == fn::copack{1ul});

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

  static_assert(not noexcept(or_else_t::apply{}(std::declval<operand_t &>(), fnThrows)));

  SUCCEED();
}

TEST_CASE("or_else identity expected", "[or_else][expected][copack]")
{
  using operand_t = fn::expected<int, fn::copack<>>;

  // the error side is uninhabited: a dedicated arm delegates to the vacuous member, which accepts
  // any callback and never instantiates it; just and choice stay excluded
  static_assert(monadic_static_check<fn::or_else_t, operand_t>::invocable_with_any(Poison{}));
  static_assert(monadic_static_check<fn::or_else_t, fn::just<int>>::not_invocable_with_any(Poison{}));
  static_assert(monadic_static_check<fn::or_else_t, fn::choice<int>>::not_invocable_with_any(Poison{}));

  operand_t a{5};
  auto r1 = a | fn::or_else(Poison{});
  static_assert(std::is_same_v<decltype(r1), operand_t>);
  CHECK(r1.value() == 5);
  auto r2 = operand_t{7} | fn::or_else(Poison{});
  CHECK(r2.value() == 7);
  static_assert((operand_t{5} | fn::or_else(Poison{})).value() == 5);

  // even a non-callable is accepted: every question this verb could ask of a callback is formed
  // with an error alternative, and there are none - not invoked, type not consulted, callability
  // not demanded
  auto r3 = a | fn::or_else(42);
  static_assert(std::is_same_v<decltype(r3), operand_t>);
  CHECK(r3.value() == 5);
  auto r4 = operand_t{9}.or_else(42);
  static_assert(std::is_same_v<decltype(r4), operand_t>);
  CHECK(r4.value() == 9);
  static_assert(std::is_same_v<decltype(fn::expected<void, fn::copack<>>{} | fn::or_else(std::declval<int>())),
                               fn::expected<void, fn::copack<>>>);
}

TEST_CASE("or_else joins heterogeneous expected branches", "[or_else][expected][copack]")
{
  struct X final {
    bool operator==(X const &) const = default;
  };
  struct Y final {
    bool operator==(Y const &) const = default;
  };
  struct E0 final {
    bool operator==(E0 const &) const = default;
  };
  struct E1 final {
    bool operator==(E1 const &) const = default;
  };
  struct E2 final {
    bool operator==(E2 const &) const = default;
  };
  constexpr auto canO = [](auto &&v, auto &&fn) { return requires { FWD(v).or_else(FWD(fn)); }; };

  SECTION("hetero recovery values need the copack-valued input, which widens in on the value path")
  {
    using In = fn::expected<fn::copack<X>, fn::copack_for<E1, E2>>;
    constexpr auto fnR = fn::overload{[](E1) { return fn::expected<X, fn::copack<E0>>{X{}}; },
                                      [](E2) { return fn::expected<Y, fn::copack<E0>>{Y{}}; }};
    auto r = In{fn::unexpect, fn::copack_for<E1, E2>{E2{}}}.or_else(fnR);
    static_assert(std::is_same_v<decltype(r), fn::expected<fn::copack_for<X, Y>, fn::copack<E0>>>);
    CHECK(r.value() == fn::copack_for<X, Y>{Y{}});
    auto v = In{fn::copack<X>{X{}}}.or_else(fnR);
    CHECK(v.value() == fn::copack_for<X, Y>{X{}});
    // the value path in every remaining value category
    In vl{fn::copack<X>{X{}}};
    CHECK(vl.or_else(fnR).value() == fn::copack_for<X, Y>{X{}});
    CHECK(std::as_const(vl).or_else(fnR).value() == fn::copack_for<X, Y>{X{}});
    CHECK(std::move(std::as_const(vl)).or_else(fnR).value() == fn::copack_for<X, Y>{X{}});
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    constexpr In cv{fn::copack<X>{X{}}};
    static_assert(cv.or_else(fnR).value() == fn::copack_for<X, Y>{X{}});

    // a plain input value with hetero recovery values answers, not errors
    using InP = fn::expected<X, fn::copack_for<E1, E2>>;
    static_assert(not canO(InP{X{}}, fnR));
    static_assert(not fn::applicable_or_else<decltype(fnR), InP>);

    // recovery branches convergent only after stripping cv/ref engage the join
    // (no constexpr twin - the reference-returning branch needs static storage, barred in
    // constant evaluation until C++23)
    constexpr auto fnRef = fn::overload{[](E1) -> fn::expected<X, fn::copack<E0>> & {
                                          static fn::expected<X, fn::copack<E0>> e{X{}};
                                          return e;
                                        },
                                        [](E2) { return fn::expected<X, fn::copack<E0>>{X{}}; }};
    auto rr = In{fn::unexpect, fn::copack_for<E1, E2>{E1{}}}.or_else(fnRef);
    static_assert(std::is_same_v<decltype(rr), fn::expected<fn::copack<X>, fn::copack<E0>>>);
    CHECK(rr.value() == fn::copack<X>{X{}});
  }

  SECTION("convergent values with hetero branch errors union; member and piped spellings agree")
  {
    using In = fn::expected<X, fn::copack_for<E1, E2>>;
    constexpr auto fnR = fn::overload{[](E1) { return fn::expected<X, E0>{X{}}; },
                                      [](E2) { return fn::expected<X, fn::copack<E2>>{fn::unexpect, E2{}}; }};
    static_assert(fn::applicable_or_else<decltype(fnR), In>); // converse of the pin above
    auto r = In{fn::unexpect, fn::copack_for<E1, E2>{E2{}}}.or_else(fnR);
    static_assert(std::is_same_v<decltype(r), fn::expected<X, fn::copack_for<E0, E2>>>);
    CHECK(r.error() == fn::copack_for<E0, E2>{E2{}});
    auto p = In{fn::unexpect, fn::copack_for<E1, E2>{E1{}}} | fn::or_else(fnR);
    static_assert(std::is_same_v<decltype(p), fn::expected<X, fn::copack_for<E0, E2>>>);
    CHECK(p.value() == X{});
  }

  SECTION("noexcept from the reachable constructions")
  {
    using In = fn::expected<X, fn::copack_for<E1, E2>>;
    In v{X{}};
    constexpr auto fnNothrow = fn::overload{[](E1) noexcept { return fn::expected<X, E0>{X{}}; },
                                            [](E2) noexcept { return fn::expected<X, E1>{X{}}; }};
    static_assert(noexcept(v.or_else(fnNothrow)));
    constexpr auto fnThrows = fn::overload{[](E1) { return fn::expected<X, E0>{X{}}; },
                                           [](E2) noexcept { return fn::expected<X, E1>{X{}}; }};
    static_assert(not noexcept(v.or_else(fnThrows)));

    // the widening arms weigh in: carrying a throwing-move value across, self's or a branch
    // result's, makes the recovery throwing even with nothrow branches
    struct ThrowingMove final {
      ThrowingMove() = default;
      ThrowingMove(ThrowingMove &&) noexcept(false) {}
      bool operator==(ThrowingMove const &) const = default;
    };
    constexpr auto fnHetero
        = fn::overload{[](E1) noexcept { return fn::expected<X, fn::copack<E0>>{X{}}; },
                       [](E2) noexcept { return fn::expected<ThrowingMove, fn::copack<E0>>{std::in_place}; }};
    using InW = fn::expected<fn::copack<X>, fn::copack_for<E1, E2>>;
    static_assert(not noexcept(std::declval<InW &&>().or_else(fnHetero)));
    using InT = fn::expected<fn::copack<ThrowingMove>, fn::copack_for<E1, E2>>;
    constexpr auto fnWiden = fn::overload{[](E1) noexcept { return fn::expected<ThrowingMove, E0>{std::in_place}; },
                                          [](E2) noexcept { return fn::expected<ThrowingMove, E0>{std::in_place}; }};
    static_assert(not noexcept(std::declval<InT &&>().or_else(fnWiden)));
    // ... while an uninhabited value side has nothing to carry, and the dead arm cannot weigh
    using InDead = fn::expected<fn::copack<>, fn::copack_for<E1, E2>>;
    static_assert(noexcept(std::declval<InDead &&>().or_else(fnNothrow)));
    SUCCEED();
  }

  SECTION("exceptions")
  {
    // carrying self's value across the recovery join may throw at runtime; the branches, which
    // recover errors only, never run on that path
    struct Boom final {
      int fuse; // the fuse-th relocation throws
      constexpr explicit Boom(int f) noexcept : fuse(f) {}
      constexpr Boom(Boom &&o) noexcept(false) : fuse(o.fuse - 1)
      {
        if (fuse == 0)
          throw 0;
      }
    };
    using InB = fn::expected<fn::copack<Boom>, fn::copack_for<E1, E2>>;
    constexpr auto fnR = fn::overload{[](E1) { return fn::expected<X, fn::copack<E0>>{X{}}; },
                                      [](E2) { return fn::expected<X, fn::copack<E0>>{X{}}; }};
    InB self{std::in_place, Boom{2}};
    CHECK_THROWS_AS(std::move(self).or_else(fnR), int);

    InB good{std::in_place, Boom{99}};
    auto r = std::move(good).or_else(fnR);
    CHECK(r.value().has_value(std::in_place_type<Boom>));
    static_assert([] {
      constexpr auto fnRX = fn::overload{[](E1) { return fn::expected<X, fn::copack<E0>>{X{}}; },
                                         [](E2) { return fn::expected<X, fn::copack<E0>>{X{}}; }};
      return InB{std::in_place, Boom{99}}.or_else(fnRX).value().has_value(std::in_place_type<Boom>);
    }());
  }

  SECTION("a plain value lifts into its singular copack")
  {
    using In = fn::expected<X, fn::copack_for<E1, E2>>;
    constexpr auto fnR = fn::overload{[](E1) { return fn::expected<X, E0>{X{}}; },
                                      [](E2) { return fn::expected<fn::copack<X>, E0>{fn::copack<X>{X{}}}; }};
    auto r = In{fn::unexpect, fn::copack_for<E1, E2>{E2{}}}.or_else(fnR);
    static_assert(std::is_same_v<decltype(r), fn::expected<fn::copack<X>, E0>>);
    CHECK(r.value() == fn::copack<X>{X{}});
    auto v = In{X{}}.or_else(fnR);
    CHECK(v.value() == fn::copack<X>{X{}});

    // the piped spelling and the concept agree with the member
    auto p = In{fn::unexpect, fn::copack_for<E1, E2>{E1{}}} | fn::or_else(fnR);
    static_assert(std::is_same_v<decltype(p), fn::expected<fn::copack<X>, E0>>);
    CHECK(p.value() == fn::copack<X>{X{}});
    static_assert(fn::applicable_or_else<decltype(fnR), In>);
  }
}

TEST_CASE("or_else joins a payload-free success", "[or_else][expected][copack]")
{
  // No default constructor: recovery must construct pack<>.
  struct Value final {
    int n;
    constexpr explicit Value(int i) noexcept : n(i) {}
    bool operator==(Value const &) const = default;
  };
  struct E0 final {
    bool operator==(E0 const &) const = default;
  };
  struct E1 final {
    bool operator==(E1 const &) const = default;
  };
  struct E2 final {
    bool operator==(E2 const &) const = default;
  };
  constexpr auto which = fn::overload{[](Value const &v) { return v.n; }, [] { return -1; },
                                      [](int i) { return 100 + i; }, [](fn::pack<>) { return -2; }};
  constexpr auto canP = [](auto &&v, auto &&fn) { return requires { FWD(v) | fn::or_else(FWD(fn)); }; };

  SECTION("plain error")
  {
    using In = fn::expected<fn::copack<Value>, E1>;
    using Out = fn::expected<fn::copack_for<Value, fn::pack<>>, E1>;
    constexpr auto fnR = [](E1) -> fn::expected<void, E1> { return {}; };
    constexpr auto fnF = [](E1) -> fn::expected<void, E2> { return ::fn::unexpected<E2>{E2{}}; };
    constexpr auto fnX = [](E1) -> fn::expected<void, E1> { throw 0; };

    In err{fn::unexpect, E1{}};
    static_assert(std::is_same_v<decltype(err.or_else(fnR)), Out>);
    static_assert(std::is_same_v<decltype(err | fn::or_else(fnR)), Out>);
    static_assert(std::is_same_v<decltype(err.or_else(fnF)), fn::expected<fn::copack_for<Value, fn::pack<>>, E2>>);

    CHECK(err.or_else(fnR).value().apply(which) == -1);
    CHECK(std::as_const(err).or_else(fnR).value().apply(which) == -1);
    CHECK(std::move(std::as_const(err)).or_else(fnR).value().apply(which) == -1);
    CHECK((err | fn::or_else(fnR)).value().apply(which) == -1);
    CHECK(err.or_else(fnF).error() == E2{});
    CHECK((err | fn::or_else(fnF)).error() == E2{});
    CHECK(std::move(err).or_else(fnR).value().apply(which) == -1);

    In val{fn::copack<Value>{Value{7}}};
    CHECK(val.or_else(fnX).value().apply(which) == 7);
    CHECK(std::as_const(val).or_else(fnX).value().apply(which) == 7);
    CHECK(std::move(std::as_const(val)).or_else(fnX).value().apply(which) == 7);
    CHECK((val | fn::or_else(fnX)).value().apply(which) == 7);
    CHECK(std::move(val).or_else(fnX).value().apply(which) == 7);

    constexpr In cerr{fn::unexpect, E1{}};
    constexpr In cval{fn::copack<Value>{Value{7}}};
    static_assert(cerr.or_else(fnR).value().apply(which) == -1);
    static_assert((cerr | fn::or_else(fnR)).value().apply(which) == -1);
    static_assert(cerr.or_else(fnF).error() == E2{});
    static_assert(cval.or_else(fnR).value().apply(which) == 7);

    using InP = fn::expected<Value, E1>;
    static_assert(not canP(InP{Value{1}}, fnR));
    static_assert(not fn::applicable_or_else<decltype(fnR), InP>);
    static_assert(fn::applicable_or_else<decltype(fnR), In>);
  }

  SECTION("coproduct error, branches mixing payload-free and valued recovery")
  {
    using Ec = fn::copack_for<E1, E2>;
    using In = fn::expected<fn::copack<Value>, Ec>;
    using Out = fn::expected<fn::copack_for<Value, fn::pack<>, int>, fn::copack_for<E0, E2>>;
    constexpr auto fnR
        = fn::overload{[](E1) -> fn::expected<void, E0> { return {}; }, [](E2) -> fn::expected<int, E2> { return 3; }};
    constexpr auto fnF = fn::overload{[](E1) -> fn::expected<void, E0> { return ::fn::unexpected<E0>{E0{}}; },
                                      [](E2) -> fn::expected<int, E2> { return 3; }};

    In e1{fn::unexpect, Ec{E1{}}};
    static_assert(std::is_same_v<decltype(e1.or_else(fnR)), Out>);
    static_assert(std::is_same_v<decltype(e1 | fn::or_else(fnR)), Out>);
    CHECK(e1.or_else(fnR).value().apply(which) == -1);
    CHECK(std::as_const(e1).or_else(fnR).value().apply(which) == -1);
    CHECK(std::move(std::as_const(e1)).or_else(fnR).value().apply(which) == -1);
    CHECK((e1 | fn::or_else(fnR)).value().apply(which) == -1);
    CHECK(e1.or_else(fnF).error() == fn::copack_for<E0, E2>{E0{}});
    CHECK(std::move(e1).or_else(fnR).value().apply(which) == -1);

    In e2{fn::unexpect, Ec{E2{}}};
    CHECK(e2.or_else(fnR).value().apply(which) == 103);
    In val{fn::copack<Value>{Value{7}}};
    CHECK(val.or_else(fnR).value().apply(which) == 7);
    CHECK((std::move(val) | fn::or_else(fnR)).value().apply(which) == 7);

    constexpr In c1{fn::unexpect, Ec{E1{}}};
    constexpr In c2{fn::unexpect, Ec{E2{}}};
    constexpr In cval{fn::copack<Value>{Value{7}}};
    static_assert(c1.or_else(fnR).value().apply(which) == -1);
    static_assert(c1.or_else(fnF).error() == fn::copack_for<E0, E2>{E0{}});
    static_assert(c2.or_else(fnR).value().apply(which) == 103);
    static_assert(cval.or_else(fnR).value().apply(which) == 7);

    constexpr auto fnI = fn::overload{[](E1) -> fn::expected<void, fn::copack<>> { return {}; },
                                      [](E2) -> fn::expected<int, E2> { return 3; }};
    using OutI = fn::expected<fn::copack_for<Value, fn::pack<>, int>, fn::copack<E2>>;
    In i1{fn::unexpect, Ec{E1{}}};
    static_assert(std::is_same_v<decltype(i1.or_else(fnI)), OutI>);
    static_assert(std::is_same_v<decltype(i1 | fn::or_else(fnI)), OutI>);
    CHECK(i1.or_else(fnI).value().apply(which) == -1);
    CHECK((i1 | fn::or_else(fnI)).value().apply(which) == -1);
    static_assert(c1.or_else(fnI).value().apply(which) == -1);
  }

  SECTION("coproduct error, convergent payload-free branches")
  {
    using Ec = fn::copack_for<E1, E2>;
    using In = fn::expected<fn::copack<Value>, Ec>;
    constexpr auto fnR = fn::overload{[](E1) -> fn::expected<void, E0> { return {}; },
                                      [](E2) -> fn::expected<void, E0> { return ::fn::unexpected<E0>{E0{}}; }};
    In e1{fn::unexpect, Ec{E1{}}};
    static_assert(std::is_same_v<decltype(e1.or_else(fnR)), fn::expected<fn::copack_for<Value, fn::pack<>>, E0>>);
    CHECK(e1.or_else(fnR).value().apply(which) == -1);
    CHECK((In{fn::unexpect, Ec{E2{}}} | fn::or_else(fnR)).error() == E0{});
    CHECK(In{fn::copack<Value>{Value{7}}}.or_else(fnR).value().apply(which) == 7);

    constexpr In c1{fn::unexpect, Ec{E1{}}};
    static_assert(c1.or_else(fnR).value().apply(which) == -1);
  }

  SECTION("an existing pack<> alternative absorbs it")
  {
    using In = fn::expected<fn::copack_for<Value, fn::pack<>>, E1>;
    constexpr auto fnR = [](E1) -> fn::expected<void, E1> { return {}; };
    In err{fn::unexpect, E1{}};
    static_assert(std::is_same_v<decltype(err.or_else(fnR)), In>);
    CHECK(err.or_else(fnR).value().apply(which) == -1);
    CHECK(In{Value{7}}.or_else(fnR).value().apply(which) == 7);
    constexpr In cerr{fn::unexpect, E1{}};
    static_assert(cerr.or_else(fnR).value().apply(which) == -1);
  }

  SECTION("the empty value grade gains pack<> alone")
  {
    using In = fn::expected<fn::copack<>, E1>;
    constexpr auto fnR = [](E1) -> fn::expected<void, E1> { return {}; };
    In err{fn::unexpect, E1{}};
    static_assert(std::is_same_v<decltype(err.or_else(fnR)), fn::expected<fn::copack<fn::pack<>>, E1>>);
    CHECK(err.or_else(fnR).value().apply(which) == -1);
    CHECK((err | fn::or_else(fnR)).value().apply(which) == -1);
    constexpr In cerr{fn::unexpect, E1{}};
    static_assert(cerr.or_else(fnR).value().apply(which) == -1);
  }

  SECTION("all-void recovery keeps void")
  {
    using In = fn::expected<void, E1>;
    constexpr auto fnR = [](E1) -> fn::expected<void, E2> { return {}; };
    static_assert(std::is_same_v<decltype(In{}.or_else(fnR)), fn::expected<void, E2>>);
    static_assert(std::is_same_v<decltype(In{} | fn::or_else(fnR)), fn::expected<void, E2>>);
    CHECK(In{fn::unexpect, E1{}}.or_else(fnR).has_value());

    using Ec = fn::copack_for<E1, E2>;
    using InC = fn::expected<void, Ec>;
    constexpr auto fnC = fn::overload{[](E1) -> fn::expected<void, E0> { return {}; },
                                      [](E2) -> fn::expected<void, E2> { return ::fn::unexpected<E2>{E2{}}; }};
    static_assert(std::is_same_v<decltype(InC{}.or_else(fnC)), fn::expected<void, fn::copack_for<E0, E2>>>);
    CHECK(InC{}.or_else(fnC).has_value());
    CHECK(InC{fn::unexpect, Ec{E1{}}}.or_else(fnC).has_value());
    CHECK(InC{fn::unexpect, Ec{E2{}}}.or_else(fnC).error() == fn::copack_for<E0, E2>{E2{}});
    constexpr InC c2{fn::unexpect, Ec{E2{}}};
    static_assert(c2.or_else(fnC).error() == fn::copack_for<E0, E2>{E2{}});
  }

  SECTION("a void value lifts into copack<pack<>>")
  {
    using In = fn::expected<void, E1>;
    using Out = fn::expected<fn::copack<fn::pack<>>, E1>;
    constexpr auto fnL = [](E1) { return Out{fn::pack<>{}}; };
    constexpr auto fnF = [](E1) { return fn::expected<fn::copack<fn::pack<>>, E2>{fn::unexpect, E2{}}; };

    In ok{};
    static_assert(std::is_same_v<decltype(ok.or_else(fnL)), Out>);
    static_assert(std::is_same_v<decltype(ok | fn::or_else(fnL)), Out>);
    CHECK(ok.or_else(fnL).value().apply(which) == -1);
    CHECK(std::as_const(ok).or_else(fnL).value().apply(which) == -1);
    CHECK(std::move(std::as_const(ok)).or_else(fnL).value().apply(which) == -1);
    CHECK((ok | fn::or_else(fnL)).value().apply(which) == -1);
    CHECK(std::move(ok).or_else(fnL).value().apply(which) == -1);
    In err{fn::unexpect, E1{}};
    CHECK(err.or_else(fnL).value().apply(which) == -1);
    CHECK(err.or_else(fnF).error() == E2{});
    CHECK((err | fn::or_else(fnF)).error() == E2{});

    using Ec = fn::copack_for<E1, E2>;
    using InC = fn::expected<void, Ec>;
    constexpr auto fnM = fn::overload{[](E1) -> fn::expected<void, E0> { return {}; },
                                      [](E2) { return fn::expected<fn::copack<fn::pack<>>, E2>{fn::unexpect, E2{}}; }};
    static_assert(
        std::is_same_v<decltype(InC{}.or_else(fnM)), fn::expected<fn::copack<fn::pack<>>, fn::copack_for<E0, E2>>>);
    InC okc{}; // Mutable for runtime coverage.
    CHECK(okc.or_else(fnM).value().apply(which) == -1);
    CHECK(InC{fn::unexpect, Ec{E1{}}}.or_else(fnM).value().apply(which) == -1);
    CHECK((InC{fn::unexpect, Ec{E2{}}} | fn::or_else(fnM)).error() == fn::copack_for<E0, E2>{E2{}});

    constexpr In cok{};
    constexpr In cerr{fn::unexpect, E1{}};
    constexpr InC cc1{fn::unexpect, Ec{E1{}}};
    static_assert(cok.or_else(fnL).value().apply(which) == -1);
    static_assert(cerr.or_else(fnL).value().apply(which) == -1);
    static_assert(cerr.or_else(fnF).error() == E2{});
    static_assert(cc1.or_else(fnM).value().apply(which) == -1);

    static_assert(canP(In{}, fnL));
    static_assert(not canP(In{}, [](E1) { return fn::expected<fn::pack<>, E1>{fn::pack<>{}}; }));
    static_assert(not canP(In{}, [](E1) { return fn::expected<fn::copack<Value>, E1>{Value{1}}; }));
    static_assert(not canP(In{}, [](E1) { return fn::expected<int, E1>{1}; }));
  }

  SECTION("move-only value")
  {
    struct MoveOnly final {
      int n;
      constexpr explicit MoveOnly(int i) noexcept : n(i) {}
      constexpr MoveOnly(MoveOnly &&) noexcept = default;
      MoveOnly(MoveOnly const &) = delete;
    };
    using In = fn::expected<fn::copack<MoveOnly>, E1>;
    using Out = fn::expected<fn::copack_for<MoveOnly, fn::pack<>>, E1>;
    constexpr auto fnR = [](E1) -> fn::expected<void, E1> { return {}; };
    constexpr auto fnX = [](E1) -> fn::expected<void, E1> { throw 0; };
    constexpr auto get = fn::overload{[](MoveOnly const &m) { return m.n; }, [] { return -1; }};
    constexpr auto canO = [](auto &&v, auto &&fn) { return requires { FWD(v).or_else(FWD(fn)); }; };

    In val{fn::copack<MoveOnly>{MoveOnly{7}}};
    static_assert(std::is_same_v<decltype(std::move(val).or_else(fnR)), Out>);
    static_assert(std::is_same_v<decltype(std::move(val) | fn::or_else(fnR)), Out>);
    static_assert(not canO(val, fnR));
    static_assert(canO(std::move(val), fnR));
    CHECK(std::move(val).or_else(fnX).value().apply(get) == 7);
    In val2{fn::copack<MoveOnly>{MoveOnly{8}}};
    CHECK((std::move(val2) | fn::or_else(fnX)).value().apply(get) == 8);
    In err{fn::unexpect, E1{}};
    CHECK(std::move(err).or_else(fnR).value().apply(get) == -1);
    In err2{fn::unexpect, E1{}};
    CHECK((std::move(err2) | fn::or_else(fnR)).value().apply(get) == -1);

    static_assert([] {
      constexpr auto fnRc = [](E1) -> fn::expected<void, E1> { return {}; };
      constexpr auto getc = fn::overload{[](MoveOnly const &m) { return m.n; }, [] { return -1; }};
      In v{fn::copack<MoveOnly>{MoveOnly{7}}};
      In e{fn::unexpect, E1{}};
      return std::move(v).or_else(fnRc).value().apply(getc) == 7
             && std::move(e).or_else(fnRc).value().apply(getc) == -1;
    }());

    using Ec = fn::copack_for<E1, E2>;
    using InC = fn::expected<fn::copack<Value>, Ec>;
    constexpr auto fnK = fn::overload{
        [](E1) noexcept -> fn::expected<void, MoveOnly> const { return fn::unexpected<MoveOnly>{MoveOnly{4}}; },
        [](E2) noexcept -> fn::expected<int, MoveOnly> { return 3; }};
    InC k1{fn::unexpect, Ec{E1{}}};
    static_assert(
        std::is_same_v<decltype(k1.or_else(fnK)), fn::expected<fn::copack_for<Value, fn::pack<>, int>, MoveOnly>>);
    static_assert(noexcept(k1.or_else(fnK)));
    CHECK(k1.or_else(fnK).error().n == 4);
    CHECK((k1 | fn::or_else(fnK)).error().n == 4);
    constexpr InC ck1{fn::unexpect, Ec{E1{}}};
    static_assert(ck1.or_else(fnK).error().n == 4);
  }

  SECTION("noexcept from the reachable constructions")
  {
    struct ThrowingMove final {
      ThrowingMove() = default;
      ThrowingMove(ThrowingMove &&) noexcept(false) {}
      bool operator==(ThrowingMove const &) const = default;
    };
    using In = fn::expected<fn::copack<Value>, E1>;
    constexpr auto fnN = [](E1) noexcept -> fn::expected<void, E1> { return {}; };
    constexpr auto fnT = [](E1) noexcept(false) -> fn::expected<void, E1> { return {}; };
    static_assert(noexcept(std::declval<In &&>().or_else(fnN)));
    static_assert(not noexcept(std::declval<In &&>().or_else(fnT)));
    static_assert(noexcept(std::declval<In &&>() | fn::or_else(fnN)));
    static_assert(not noexcept(std::declval<In &&>() | fn::or_else(fnT)));
    static_assert(not noexcept(std::declval<fn::expected<fn::copack<ThrowingMove>, E1> &&>().or_else(fnN)));
    constexpr auto fnTE = [](E1) noexcept -> fn::expected<void, ThrowingMove> { return {}; };
    static_assert(not noexcept(std::declval<In &&>().or_else(fnTE)));

    using InC = fn::expected<fn::copack<Value>, fn::copack_for<E1, E2>>;
    constexpr auto fnC = fn::overload{[](E1) noexcept -> fn::expected<void, E0> { return {}; },
                                      [](E2) noexcept -> fn::expected<int, E2> { return 3; }};
    static_assert(noexcept(std::declval<InC &&>().or_else(fnC)));
    constexpr auto fnCT = fn::overload{[](E1) noexcept -> fn::expected<void, ThrowingMove> { return {}; },
                                       [](E2) noexcept -> fn::expected<int, E2> { return 3; }};
    static_assert(not noexcept(std::declval<InC &&>().or_else(fnCT)));
    constexpr auto fnCI = fn::overload{[](E1) noexcept -> fn::expected<void, fn::copack<>> { return {}; },
                                       [](E2) noexcept -> fn::expected<int, E2> { return 3; }};
    static_assert(noexcept(std::declval<InC &&>().or_else(fnCI)));

    using V0 = fn::expected<void, E1>;
    constexpr auto fnL = [](E1) noexcept { return fn::expected<fn::copack<fn::pack<>>, E1>{fn::pack<>{}}; };
    constexpr auto fnLT = [](E1) noexcept(false) { return fn::expected<fn::copack<fn::pack<>>, E1>{fn::pack<>{}}; };
    static_assert(noexcept(std::declval<V0 &>().or_else(fnL)));
    static_assert(not noexcept(std::declval<V0 &>().or_else(fnLT)));
    SUCCEED();
  }

  SECTION("exceptions")
  {
    struct Boom final {
      int fuse;
      constexpr explicit Boom(int f) noexcept : fuse(f) {}
      constexpr Boom(Boom &&o) noexcept(false) : fuse(o.fuse - 1)
      {
        if (fuse == 0)
          throw 0;
      }
    };
    using Ec = fn::copack_for<E1, E2>;
    using In = fn::expected<fn::copack<Value>, Ec>;
    // fuse 2: the branch builds its error, and entering it into the joined error throws
    constexpr auto fnArmed = fn::overload{[](E1) { return fn::expected<void, Boom>{fn::unexpect, Boom{2}}; },
                                          [](E2) -> fn::expected<int, E0> { return 3; }};
    In self{fn::unexpect, Ec{E1{}}};
    CHECK_THROWS_AS(self.or_else(fnArmed), int);
    CHECK(self.error() == Ec{E1{}});

    constexpr auto fnSafe = fn::overload{[](E1) { return fn::expected<void, Boom>{fn::unexpect, Boom{99}}; },
                                         [](E2) -> fn::expected<int, E0> { return 3; }};
    CHECK(self.or_else(fnSafe).error().has_value(std::in_place_type<Boom>));
    static_assert([] {
      constexpr auto fnS = fn::overload{[](E1) { return fn::expected<void, Boom>{fn::unexpect, Boom{99}}; },
                                        [](E2) -> fn::expected<int, E0> { return 3; }};
      In s{fn::unexpect, Ec{E1{}}};
      return s.or_else(fnS).error().has_value(std::in_place_type<Boom>);
    }());
  }
}

TEST_CASE("or_else tuple-like error payload", "[or_else][expected][tuple]")
{
  // a lone tuple-like error exposes its elements to the recovery callback, member and functor alike
  using TE = std::tuple<int, int>;
  constexpr auto fnR = [](int a, int b) noexcept { return fn::expected<bool, TE>{a + b == 42}; };

  fn::expected<bool, TE> e{fn::unexpect, TE{20, 22}};
  CHECK(e.or_else(fnR).value());
  CHECK((e | fn::or_else(fnR)).value());

  SECTION("constexpr")
  {
    static_assert(fn::expected<bool, TE>{fn::unexpect, TE{20, 22}}.or_else(fnR).value());
    static_assert((fn::expected<bool, TE>{fn::unexpect, TE{20, 22}} | fn::or_else(fnR)).value());
    SUCCEED();
  }

  SECTION("a coproduct alternative's elements reach the branch, past a whole-tuple overload")
  {
    struct E2 final {};
    using T1 = std::tuple<int>;
    using In = fn::expected<fn::copack<int>, fn::copack_for<T1, E2>>;
    constexpr auto fnV = fn::overload{[](T1) { return 91; }, [](int i) noexcept -> fn::expected<int, int> { return i; },
                                      [](E2) noexcept -> fn::expected<long, int> { return 2L; }};
    constexpr auto fnP = fn::overload{[](T1) { return 91; },
                                      [](int i) noexcept -> fn::expected<void, int> {
                                        if (i == 3)
                                          return {};
                                        return fn::unexpected<int>{i};
                                      },
                                      [](E2) noexcept -> fn::expected<long, int> { return 2L; }};
    constexpr auto which = fn::overload{[](int i) { return i; }, [](long) { return -2; }, [] { return -1; }};
    using OutP = fn::expected<fn::copack_for<fn::pack<>, int, long>, int>;

    In e3{fn::unexpect, T1{3}};
    In e4{fn::unexpect, T1{4}};
    static_assert(std::is_same_v<decltype(e3.or_else(fnP)), OutP>);
    static_assert(std::is_same_v<decltype(e3 | fn::or_else(fnP)), OutP>);
    CHECK(e3.or_else(fnV).value().apply(which) == 3);
    CHECK((e3 | fn::or_else(fnV)).value().apply(which) == 3);
    CHECK(e3.or_else(fnP).value().apply(which) == -1);
    CHECK((e3 | fn::or_else(fnP)).value().apply(which) == -1);
    CHECK(e4.or_else(fnP).error() == 4);

    constexpr In c3{fn::unexpect, T1{3}};
    constexpr In c4{fn::unexpect, T1{4}};
    static_assert(c3.or_else(fnV).value().apply(which) == 3);
    static_assert(c3.or_else(fnP).value().apply(which) == -1);
    static_assert((c3 | fn::or_else(fnP)).value().apply(which) == -1);
    static_assert(c4.or_else(fnP).error() == 4);
  }

  SECTION("the overload the join is typed from is the one invoked")
  {
    struct Pinned final {
      int n;
      constexpr explicit Pinned(int i) noexcept : n(i) {}
      constexpr explicit Pinned(Pinned const &o) noexcept : n(o.n) {}
      constexpr Pinned(Pinned &&) noexcept = default;
    };
    struct E2 final {};
    using T1 = std::tuple<int>;
    using In = fn::expected<fn::copack<int>, fn::copack_for<T1, E2>>;
    constexpr auto run = [] {
      fn::expected<Pinned, int> recovery{std::in_place, 3};
      int elements = 0;
      int whole = 0;
      auto const fnR = fn::overload{[&](int) noexcept -> fn::expected<Pinned, int> & {
                                      ++elements;
                                      return recovery;
                                    },
                                    [&](T1) noexcept -> fn::expected<long, int> {
                                      ++whole;
                                      return 91L;
                                    },
                                    [](E2) noexcept -> fn::expected<long, int> { return 2L; }};
      auto const which
          = fn::overload{[](Pinned const &p) { return p.n; }, [](int) { return -1; }, [](long) { return -2; }};
      In e{fn::unexpect, T1{3}};
      static_assert(noexcept(e.or_else(fnR)));
      static_assert(noexcept(e | fn::or_else(fnR)));
      return e.or_else(fnR).value().apply(which) == 3 && (e | fn::or_else(fnR)).value().apply(which) == 3
             && elements == 2 && whole == 0;
    };
    CHECK(run());
    static_assert(run());
  }
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
static_assert(applicable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<Value, Error>>);
static_assert(applicable_or_else<decltype(fn_generic<expected<int, int>>), expected<int, int>>);
static_assert(applicable_or_else<decltype(fn_Error<expected<Value, Xerror>>), expected<Value, Error>>);   // error type conversion
static_assert(applicable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<Value, Xerror>>);   // error type conversion
static_assert(applicable_or_else<decltype(fn_Error<expected<Value, int>>), expected<Value, Xerror>>);     // error type conversion
static_assert(not applicable_or_else<decltype(fn_Error<expected<Value, int>>), expected<Value, int>>);    // wrong error_type
static_assert(applicable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<Value, Error>>);
static_assert(not applicable_or_else<decltype(fn_Error<expected<Value, Error>>), expected<void, Error>>); // cannot change value_type
static_assert(not applicable_or_else<decltype(fn_Error<expected<void, Error>>), expected<Value, Error>>); // cannot change value_type
static_assert(not applicable_or_else<decltype(fn_generic<expected<Value, int>>), expected<int, int>>);    // cannot change value_type

static_assert(not applicable_or_else<decltype(fn_generic<expected<Value, Error>>), optional<Value>>);     // mixed optional and expected
static_assert(not applicable_or_else<decltype(fn_generic<optional<Value>>), expected<Value, Error>>);     // mixed optional and expected
static_assert(applicable_or_else<decltype(fn_generic<optional<Value>>), optional<Value>>);
static_assert(not applicable_or_else<decltype(fn_generic<optional<int>>), optional<Value>>);              // cannot change value_type
static_assert(not applicable_or_else<decltype(fn_generic<optional<Value>>), optional<int>>);              // cannot change value_type

static_assert(not applicable_or_else<decltype(fn_int_lvalue<expected<int, int>>), expected<int, int>>);   // cannot bind temporary to lvalue
static_assert(applicable_or_else<decltype(fn_int_lvalue<expected<int, int>>), expected<int, int> &>);
static_assert(applicable_or_else<decltype(fn_int_rvalue<expected<int, int>>), expected<int, int>>);
static_assert(not applicable_or_else<decltype(fn_int_rvalue<expected<int, int>>), expected<int, int> &>); // cannot bind lvalue to rvalue-ref

// at an uninhabited error side the concept stays false - the dedicated arm, not the concept, admits the operand
static_assert(not applicable_or_else<decltype(fn_generic<expected<Value, copack<>>>), expected<Value, copack<>>>);
// clang-format on
} // namespace fn

TEST_CASE("or_else across expected and optional", "[or_else][expected][optional][copack]")
{
  struct A final {
    bool operator==(A const &) const = default;
  };
  struct E1 final {
    bool operator==(E1 const &) const = default;
  };
  struct E2 final {
    bool operator==(E2 const &) const = default;
  };
  constexpr auto can = [](auto &&v, auto &&fn) { return requires { FWD(v) | fn::or_else(FWD(fn)); }; };

  SECTION("optional to expected: the empty state invokes, the value passes through")
  {
    constexpr auto fnE = []() { return fn::expected<int, E1>{fn::unexpect, E1{}}; };
    auto r1 = fn::optional<int>{5} | fn::or_else(fnE);
    static_assert(std::is_same_v<decltype(r1), fn::expected<int, E1>>);
    CHECK(r1.value() == 5);
    auto r2 = fn::optional<int>{} | fn::or_else(fnE);
    CHECK(r2.error() == E1{});
    // a mutable source defeats the front end's constant folding of the whole prvalue pipeline
    // above, so the disengaged arm genuinely executes and gcov sees it
    fn::optional<int> mut{};
    CHECK((mut | fn::or_else(fnE)).error() == E1{});
    // the pass-through in every remaining value category
    fn::optional<int> vl{7};
    CHECK((vl | fn::or_else(fnE)).value() == 7);
    CHECK((std::as_const(vl) | fn::or_else(fnE)).value() == 7);
    CHECK((std::move(std::as_const(vl)) | fn::or_else(fnE)).value() == 7);
    CHECK((std::move(vl) | fn::or_else(fnE)).value() == 7);
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    constexpr fn::optional<int> cv{5};
    static_assert((cv | fn::or_else(fnE)).value() == 5);
    static_assert((fn::optional<int>{} | fn::or_else(fnE)).error() == E1{});
  }

  SECTION("optional to expected: graded targets and the sticky value grade")
  {
    // the callback's graded error passes as spelled; recovery to identity is admitted
    auto r1 = fn::optional<int>{1} | fn::or_else([]() { return fn::expected<int, fn::copack<E1>>{1}; });
    static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::copack<E1>>>);
    CHECK(r1.value() == 1);
    auto r2 = fn::optional<int>{} | fn::or_else([]() { return fn::expected<int, fn::copack<>>{7}; });
    static_assert(std::is_same_v<decltype(r2), fn::expected<int, fn::copack<>>>);
    CHECK(r2.value() == 7);
    // a graded self value acquires the branch value, in both states
    constexpr auto fnA = []() { return fn::expected<A, E1>{A{}}; };
    auto r3 = fn::optional<fn::copack<int>>{} | fn::or_else(fnA);
    static_assert(std::is_same_v<decltype(r3), fn::expected<fn::copack_for<int, A>, E1>>);
    CHECK(r3.value() == fn::copack_for<int, A>{A{}});
    auto r4 = fn::optional<fn::copack<int>>{fn::copack<int>{3}} | fn::or_else(fnA);
    CHECK(r4.value() == fn::copack_for<int, A>{3});
    // the empty value grade converts outright - its callback always runs
    auto r5 = fn::optional<fn::copack<>>{} | fn::or_else([]() { return fn::expected<int, E1>{2}; });
    static_assert(std::is_same_v<decltype(r5), fn::expected<fn::copack<int>, E1>>);
    CHECK(r5.value() == fn::copack<int>{2});
  }

  SECTION("expected to optional: the error invokes, per alternative when graded")
  {
    constexpr auto fnO = [](E1) { return fn::optional<int>{}; };
    auto r1 = fn::expected<int, E1>{9} | fn::or_else(fnO);
    static_assert(std::is_same_v<decltype(r1), fn::optional<int>>);
    CHECK(r1.value() == 9);
    auto r2 = fn::expected<int, E1>{fn::unexpect, E1{}} | fn::or_else(fnO);
    CHECK(not r2.has_value());
    constexpr fn::expected<int, E1> ce{9};
    static_assert((ce | fn::or_else(fnO)).value() == 9);
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    constexpr fn::expected<int, E1> cerr{fn::unexpect, E1{}};
    static_assert(not(cerr | fn::or_else(fnO)).has_value());
    // graded dispatch reaches each branch; the value passes through
    constexpr auto fnB = fn::overload{[](E1) { return fn::optional<int>{}; }, //
                                      [](E2) { return fn::optional<int>{3}; }};
    using In = fn::expected<int, fn::copack_for<E1, E2>>;
    CHECK(not(In{fn::unexpect, fn::copack_for<E1, E2>{E1{}}} | fn::or_else(fnB)).has_value());
    CHECK((In{fn::unexpect, fn::copack_for<E1, E2>{E2{}}} | fn::or_else(fnB)).value() == 3);
    CHECK((In{5} | fn::or_else(fnB)).value() == 5);
    // the sticky grade crosses the kind switch
    auto r3 = fn::expected<fn::copack<>, E1>{fn::unexpect, E1{}} | fn::or_else([](E1) { return fn::optional<int>{4}; });
    static_assert(std::is_same_v<decltype(r3), fn::optional<fn::copack<int>>>);
    CHECK(r3.value() == fn::copack<int>{4});
    // heterogeneous branch values join under the sticky grade
    using InG = fn::expected<fn::copack<A>, fn::copack_for<E1, E2>>;
    constexpr auto fnH = fn::overload{[](E1) { return fn::optional<int>{1}; },
                                      [](E2) { return fn::optional<fn::copack<int>>{fn::copack<int>{2}}; }};
    auto r4 = InG{fn::unexpect, fn::copack_for<E1, E2>{E2{}}} | fn::or_else(fnH);
    static_assert(std::is_same_v<decltype(r4), fn::optional<fn::copack_for<A, int>>>);
    CHECK(r4.value() == fn::copack_for<A, int>{2});
    CHECK((InG{fn::copack<A>{A{}}} | fn::or_else(fnH)).value() == fn::copack_for<A, int>{A{}});
  }

  SECTION("optional to expected: a payload-free recovery joins the grade as pack<>")
  {
    using In = fn::optional<fn::copack<A>>;
    using Value = fn::copack_for<A, fn::pack<>>;
    constexpr auto fnR = []() { return fn::expected<void, E1>{}; };
    constexpr auto fnF = []() { return fn::expected<void, E1>{fn::unexpect, E1{}}; };
    constexpr auto fnX = []() -> fn::expected<void, E1> { throw 0; };
    static_assert(std::is_same_v<decltype(In{} | fn::or_else(fnR)), fn::expected<Value, E1>>);
    In mut{};
    CHECK((mut | fn::or_else(fnR)).value() == Value{fn::pack<>{}});
    CHECK((mut | fn::or_else(fnF)).error() == E1{});
    In vl{fn::copack<A>{A{}}};
    CHECK((vl | fn::or_else(fnX)).value() == Value{A{}});
    CHECK((std::as_const(vl) | fn::or_else(fnX)).value() == Value{A{}});
    CHECK((std::move(std::as_const(vl)) | fn::or_else(fnX)).value() == Value{A{}});
    CHECK((std::move(vl) | fn::or_else(fnX)).value() == Value{A{}});
    constexpr In cempty{};
    constexpr In cv{fn::copack<A>{A{}}};
    static_assert((cempty | fn::or_else(fnR)).value() == Value{fn::pack<>{}});
    static_assert((cempty | fn::or_else(fnF)).error() == E1{});
    static_assert((cv | fn::or_else(fnR)).value() == Value{A{}});

    static_assert(noexcept(std::declval<In &>() | fn::or_else([]() noexcept { return fn::expected<void, E1>{}; })));
    static_assert(not noexcept(std::declval<In &>() | fn::or_else(fnR)));
    static_assert(can(In{}, fnR));
    static_assert(not can(fn::optional<A>{}, fnR));
  }

  SECTION("expected to optional: a void value lifts into copack<pack<>>")
  {
    using In = fn::expected<void, E1>;
    using Out = fn::optional<fn::copack<fn::pack<>>>;
    constexpr auto fnO = [](E1) { return Out{fn::pack<>{}}; };
    constexpr auto fnN = [](E1) { return Out{}; };
    static_assert(std::is_same_v<decltype(In{} | fn::or_else(fnO)), Out>);
    In ok{};
    CHECK((ok | fn::or_else(fnN)).value() == Out::value_type{fn::pack<>{}});
    CHECK((std::as_const(ok) | fn::or_else(fnN)).value() == Out::value_type{fn::pack<>{}});
    CHECK((std::move(std::as_const(ok)) | fn::or_else(fnN)).value() == Out::value_type{fn::pack<>{}});
    CHECK((std::move(ok) | fn::or_else(fnN)).value() == Out::value_type{fn::pack<>{}});
    In err{fn::unexpect, E1{}};
    CHECK((err | fn::or_else(fnO)).value() == Out::value_type{fn::pack<>{}});
    CHECK(not(err | fn::or_else(fnN)).has_value());

    using Ec = fn::copack_for<E1, E2>;
    using InC = fn::expected<void, Ec>;
    constexpr auto fnB = fn::overload{[](E1) { return Out{fn::pack<>{}}; }, [](E2) { return Out{}; }};
    CHECK((InC{} | fn::or_else(fnB)).value() == Out::value_type{fn::pack<>{}});
    CHECK((InC{fn::unexpect, Ec{E1{}}} | fn::or_else(fnB)).value() == Out::value_type{fn::pack<>{}});
    CHECK(not(InC{fn::unexpect, Ec{E2{}}} | fn::or_else(fnB)).has_value());

    constexpr In cok{};
    constexpr In cerr{fn::unexpect, E1{}};
    constexpr InC cc2{fn::unexpect, Ec{E2{}}};
    static_assert((cok | fn::or_else(fnN)).value() == Out::value_type{fn::pack<>{}});
    static_assert((cerr | fn::or_else(fnO)).value() == Out::value_type{fn::pack<>{}});
    static_assert(not(cc2 | fn::or_else(fnB)).has_value());

    static_assert(noexcept(std::declval<In &>() | fn::or_else([](E1) noexcept { return Out{}; })));
    static_assert(not noexcept(std::declval<In &>() | fn::or_else(fnN)));
    static_assert(can(In{}, fnO));
    static_assert(not can(In{}, [](E1) { return fn::optional<int>{}; }));
    static_assert(not can(In{}, [](E1) { return fn::optional<fn::pack<>>{}; }));
  }

  SECTION("refusals answer, and their converses hold")
  {
    static_assert(can(fn::expected<int, E1>{1}, [](E1) { return fn::optional<int>{}; }));    // the converse
    static_assert(not can(fn::expected<int, E1>{1}, [](E1) { return fn::just<int>{1}; }));   // cluster targets
    static_assert(not can(fn::expected<int, E1>{1}, [](E1) { return fn::choice<int>{1}; })); // belong to and_then
    static_assert(not can(fn::optional<int>{}, []() { return fn::just<int>{1}; }));
    static_assert(not can(fn::optional<int>{}, []() { return 1; })); // bare values stay transform's
    static_assert(not can(fn::optional<int>{}, 42));                 // the 1-state asks invocability - and refuses
    // a plain self value must converge with the branch value exactly
    static_assert(not can(fn::optional<int>{}, []() { return fn::expected<A, E1>{A{}}; }));
    static_assert(
        fn::applicable_or_else_across<decltype([](E1) { return fn::optional<int>{}; }) &&, fn::expected<int, E1> &&>);
    static_assert(not fn::applicable_or_else_across<decltype([]() { return fn::optional<int>{}; }) &&,
                                                    fn::expected<int, fn::copack<>> &&>);
    SUCCEED();
  }

  SECTION("noexcept and exceptions")
  {
    constexpr auto fnE = []() noexcept { return fn::expected<int, E1>{fn::unexpect, E1{}}; };
    static_assert(noexcept(std::declval<fn::optional<int> &>() | fn::or_else(fnE)));
    constexpr auto fnT = []() { return fn::expected<int, E1>{fn::unexpect, E1{}}; };
    static_assert(not noexcept(std::declval<fn::optional<int> &>() | fn::or_else(fnT))); // callback may throw
    // the pass-through relocation weighs in, and throws at runtime when armed
    struct Boom {
      int fuse; // defined, not just declared
      constexpr explicit Boom(int f) : fuse(f) {}
      constexpr Boom(Boom &&o) noexcept(false) : fuse(o.fuse - 1)
      {
        if (fuse == 0)
          throw 0;
      }
      constexpr Boom(Boom const &o) noexcept(false) : fuse(o.fuse - 1)
      {
        if (fuse == 0)
          throw 0;
      }
    };
    constexpr auto fnBm = []() { return fn::expected<Boom, E1>{fn::unexpect, E1{}}; };
    static_assert(not noexcept(std::declval<fn::optional<Boom> &>() | fn::or_else(fnBm)));
    fn::optional<Boom> src{std::in_place, Boom{2}}; // in-place relocation burns one: fuse is 1
    CHECK_THROWS_AS((void)(std::move(src) | fn::or_else(fnBm)), int);
    CHECK(src.has_value()); // the throw happened constructing the result; self still holds its state
    fn::optional<Boom> ok{std::in_place, Boom{3}}; // fuse 2: the pass-through completes on 1
    CHECK((std::move(ok) | fn::or_else(fnBm)).value().fuse == 1);
    static_assert([] { // the completing twin, replayed in constant evaluation
      fn::optional<Boom> o{std::in_place, Boom{3}};
      return (std::move(o) | fn::or_else([]() { return fn::expected<Boom, E1>{fn::unexpect, E1{}}; })).value().fuse
             == 1;
    }());
  }

  SECTION("optional to expected: a recovery returned by reference is copied, and the copy weighs in")
  {
    struct Fragile final {
      bool armed;
      constexpr explicit Fragile(bool a) noexcept : armed(a) {}
      constexpr Fragile(Fragile &&) noexcept = default;
      constexpr Fragile(Fragile const &o) noexcept(false) : armed(o.armed)
      {
        if (armed)
          throw 0;
      }
      bool operator==(Fragile const &) const = default;
    };
    using In = fn::optional<fn::copack<A>>;
    fn::expected<void, Fragile> rv{fn::unexpect, Fragile{true}};
    fn::expected<int, Fragile> ri{fn::unexpect, Fragile{true}};
    auto const fnV = [&]() noexcept -> fn::expected<void, Fragile> & { return rv; };
    auto const fnI = [&]() noexcept -> fn::expected<int, Fragile> & { return ri; };
    static_assert(not noexcept(std::declval<In &>() | fn::or_else(fnV)));
    static_assert(not noexcept(std::declval<In &>() | fn::or_else(fnI)));
    In empty{};
    CHECK_THROWS_AS((void)(empty | fn::or_else(fnV)), int);
    CHECK_THROWS_AS((void)(empty | fn::or_else(fnI)), int);
    using Same = fn::expected<fn::copack<A>, Fragile>;
    Same rs{fn::unexpect, Fragile{true}};
    auto const fnS = [&]() noexcept -> Same & { return rs; };
    static_assert(std::is_same_v<decltype(empty | fn::or_else(fnS)), Same>);
    static_assert(not noexcept(std::declval<In &>() | fn::or_else(fnS)));
    CHECK_THROWS_AS((void)(empty | fn::or_else(fnS)), int);

    fn::expected<void, Fragile> sv{fn::unexpect, Fragile{false}};
    fn::expected<int, Fragile> si{5};
    Same ss{fn::unexpect, Fragile{false}};
    auto const fnSV = [&]() noexcept -> fn::expected<void, Fragile> & { return sv; };
    auto const fnSI = [&]() noexcept -> fn::expected<int, Fragile> & { return si; };
    auto const fnSS = [&]() noexcept -> Same & { return ss; };
    CHECK((empty | fn::or_else(fnSV)).error() == Fragile{false});
    CHECK((empty | fn::or_else(fnSI)).value() == fn::copack_for<A, int>{5});
    CHECK((empty | fn::or_else(fnSS)).error() == Fragile{false});
    static_assert([] {
      fn::expected<void, Fragile> r{fn::unexpect, Fragile{false}};
      In e{};
      return (e | fn::or_else([&]() noexcept -> fn::expected<void, Fragile> & { return r; })).error() == Fragile{false};
    }());

    fn::expected<void, E1> nv{};
    fn::expected<int, E1> ni{1};
    fn::expected<fn::copack<A>, E1> ns{fn::unexpect, E1{}};
    static_assert(noexcept(std::declval<In &>()
                           | fn::or_else([&]() noexcept -> fn::expected<fn::copack<A>, E1> & { return ns; })));
    static_assert(
        noexcept(std::declval<In &>() | fn::or_else([&]() noexcept -> fn::expected<void, E1> & { return nv; })));
    static_assert(
        noexcept(std::declval<In &>() | fn::or_else([&]() noexcept -> fn::expected<int, E1> & { return ni; })));
    // Copy elision bypasses the throwing move.
    struct MoveMayThrow final {
      MoveMayThrow() = default;
      MoveMayThrow(MoveMayThrow &&) noexcept(false) {}
    };
    static_assert(noexcept(std::declval<In &>() | fn::or_else([]() noexcept {
                             return fn::expected<fn::copack<A>, MoveMayThrow>{fn::unexpect};
                           })));
  }

  SECTION("optional to expected: a const prvalue result is moved from")
  {
    struct MoveOnly final {
      int n;
      constexpr explicit MoveOnly(int i) noexcept : n(i) {}
      constexpr MoveOnly(MoveOnly &&) noexcept = default;
      MoveOnly(MoveOnly const &) = delete;
    };
    using In = fn::optional<fn::copack<A>>;
    constexpr auto fnV
        = []() noexcept -> fn::expected<MoveOnly, E1> const { return fn::expected<MoveOnly, E1>{std::in_place, 7}; };
    constexpr auto fnE
        = []() noexcept -> fn::expected<void, MoveOnly> const { return fn::unexpected<MoveOnly>{MoveOnly{8}}; };
    constexpr auto get = fn::overload{[](A const &) { return -1; }, [](MoveOnly const &m) { return m.n; }};
    static_assert(std::is_same_v<decltype(In{} | fn::or_else(fnV)), fn::expected<fn::copack_for<A, MoveOnly>, E1>>);
    static_assert(
        std::is_same_v<decltype(In{} | fn::or_else(fnE)), fn::expected<fn::copack_for<A, fn::pack<>>, MoveOnly>>);
    static_assert(noexcept(std::declval<In &>() | fn::or_else(fnV)));
    static_assert(noexcept(std::declval<In &>() | fn::or_else(fnE)));
    In empty{};
    CHECK((empty | fn::or_else(fnV)).value().apply(get) == 7);
    CHECK((empty | fn::or_else(fnE)).error().n == 8);
    constexpr In cempty{};
    static_assert((cempty | fn::or_else(fnV)).value().apply(get) == 7);
    static_assert((cempty | fn::or_else(fnE)).error().n == 8);
  }
}
