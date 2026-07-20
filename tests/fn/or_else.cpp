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
