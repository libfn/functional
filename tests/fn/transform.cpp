// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/functor.hpp>
#include <fn/transform.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <utility>

using namespace util;

namespace {
struct Error final {
  std::string what;
};

struct Xint final {
  int value;
};
} // namespace

TEST_CASE("transform", "[transform][expected][expected_value][pack]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using is = monadic_static_check<transform_t, operand_t>;

  constexpr auto fnValue = [](int i) -> int { return i + 1; };
  constexpr auto wrong = [](int) -> int { throw 0; };
  constexpr auto fnXabs = [](int i) -> Xint { return {std::abs(8 - i)}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> int { throw 0; }));                    // allow generic call
  static_assert(is::invocable_with_any([](int) -> int { throw 0; }));                        // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> int { throw 0; }));                   // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> int { throw 0; }));                // binds to const ref
  static_assert(is::invocable<lvalue>([](int &) -> int { throw 0; }));                       // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](int &&) -> int { throw 0; }));             // can move
  static_assert(is::invocable<rvalue, crvalue>([](int const &&) -> int { throw 0; }));       // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](int &) -> int { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](int &) -> int { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) -> int { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> int { throw 0; }));                     // bad type
  static_assert(is::not_invocable_with_any([]() -> int { throw 0; }));                                // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> int { throw 0; }));                        // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((a | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is pack")
  {
    using operand_t = fn::expected<fn::pack<int, double>, Error>;
    operand_t a{std::in_place, fn::pack{84, 0.5}};
    constexpr auto fnPack = [](int i, double d) constexpr -> int { return i * d; };
    using T = decltype(a | transform(fnPack));
    static_assert(std::is_same_v<T, fn::expected<int, Error>>);
    WHEN("operand is value") { REQUIRE((a | transform(fnPack)).value() == 42); }

    WHEN("operand is error")
    {
      constexpr auto wrong = [](auto...) -> int { throw 0; };
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | transform(wrong)).error().what == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place, 12} | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place, 12} | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("transform noexcept", "[transform][expected][expected_value][noexcept]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;

  constexpr auto fnNothrow = [](int i) noexcept -> int { return i + 1; };
  constexpr auto fnThrows = [](int i) noexcept(false) -> int { return i + 1; };

  // The member weighs the callback AND the copy of the untouched error. Error carries a std::string,
  // whose copy can throw, so even a noexcept callback leaves the member potentially-throwing.
  static_assert(not std::is_nothrow_copy_constructible_v<Error>);
  static_assert(not noexcept(std::declval<operand_t &>().transform(fnNothrow)));

  // Give it an error whose copy cannot throw, and the callback alone decides.
  using nothrow_t = fn::expected<int, int>;
  static_assert(noexcept(std::declval<nothrow_t &>().transform(fnNothrow)));
  static_assert(not noexcept(std::declval<nothrow_t &>().transform(fnThrows)));

  // GAP #285: transform_t::apply discards that, being unconditionally noexcept - as is the rest of
  // the pipeline it is reached through (pinned in tests/fn/functor.cpp).
  static_assert(noexcept(transform_t::apply{}(std::declval<nothrow_t &>(), fnThrows)));

  SUCCEED();
}

TEST_CASE("transform", "[transform][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using is = monadic_static_check<transform_t, operand_t>;

  int count = 0;
  auto fnValue = [&count]() -> void { count += 1; };
  constexpr auto wrong = []() -> void { throw 0; };
  constexpr auto fnXabs = []() -> Xint { return {42}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> int { throw 0; }));        // allow generic call
  static_assert(is::invocable_with_any([]() -> Xint { throw 0; }));              // allow conversion
  static_assert(is::not_invocable_with_any([](auto) -> int { throw 0; }));       // bad arity
  static_assert(is::not_invocable_with_any([](auto, auto) -> int { throw 0; })); // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (a | transform(fnValue)).value();
      REQUIRE(count == 1);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((a | transform(fnXabs)).value().value == 42);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | transform(fnValue)).value();
      REQUIRE(count == 1);

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place} | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place} | transform(fnXabs)).value().value == 42);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
               | transform(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("transform noexcept", "[transform][expected][expected_void][noexcept]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;

  constexpr auto fnNothrow = []() noexcept -> int { return 1; };
  constexpr auto fnThrows = []() noexcept(false) -> int { return 1; };

  // As for a non-void value: the untouched Error's copy can throw, so isolate the callback with an
  // error whose copy cannot.
  static_assert(not noexcept(std::declval<operand_t &>().transform(fnNothrow)));

  using nothrow_t = fn::expected<void, int>;
  static_assert(noexcept(std::declval<nothrow_t &>().transform(fnNothrow)));
  static_assert(not noexcept(std::declval<nothrow_t &>().transform(fnThrows)));

  static_assert(noexcept(transform_t::apply{}(std::declval<nothrow_t &>(), fnThrows))); // GAP #285

  SUCCEED();
}

TEST_CASE("transform", "[transform][optional][pack]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  using is = monadic_static_check<transform_t, operand_t>;

  constexpr auto fnValue = [](int i) -> int { return i + 1; };
  constexpr auto wrong = [](int) -> int { throw 0; };
  constexpr auto fnXabs = [](int i) -> Xint { return {std::abs(8 - i)}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> int { throw 0; }));                    // allow generic call
  static_assert(is::invocable_with_any([](int) -> int { throw 0; }));                        // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> int { throw 0; }));                   // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> int { throw 0; }));                // binds to const ref
  static_assert(is::invocable<lvalue>([](int &) -> int { throw 0; }));                       // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](int &&) -> int { throw 0; }));             // can move
  static_assert(is::invocable<rvalue, crvalue>([](int const &&) -> int { throw 0; }));       // binds to const rvalue
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>([](int &) -> int { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](int &) -> int { throw 0; }));                   // disallow bind
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) -> int { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> int { throw 0; }));                     // bad type
  static_assert(is::not_invocable_with_any([]() -> int { throw 0; }));                                // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> int { throw 0; }));                        // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::optional<Xint>>);
        REQUIRE((a | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | transform(wrong)).has_value());
    }
  }

  WHEN("operand is pack")
  {
    using operand_t = fn::optional<fn::pack<int, double>>;
    operand_t a{std::in_place, fn::pack{84, 0.5}};
    constexpr auto fnPack = [](int i, double d) constexpr -> int { return i * d; };
    using T = decltype(a | transform(fnPack));
    static_assert(std::is_same_v<T, fn::optional<int>>);

    WHEN("operand is value") { REQUIRE((a | transform(fnPack)).value() == 42); }

    WHEN("operand is error")
    {
      constexpr auto wrong = [](auto...) -> int { throw 0; };
      REQUIRE(not(operand_t{std::nullopt} | transform(wrong)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | transform(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | transform(fnValue)).value() == 13);

      WHEN("change type")
      {
        using T = decltype(operand_t{12} | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::optional<Xint>>);
        REQUIRE((operand_t{12} | transform(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | transform(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | transform(wrong))
                     .has_value());
    }
  }
}

TEST_CASE("transform noexcept", "[transform][optional][noexcept]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;

  constexpr auto fnNothrow = [](int i) noexcept -> int { return i + 1; };
  constexpr auto fnThrows = [](int i) noexcept(false) -> int { return i + 1; };

  // An optional has no error side to copy, so the callback alone decides.
  static_assert(noexcept(std::declval<operand_t &>().transform(fnNothrow)));
  static_assert(not noexcept(std::declval<operand_t &>().transform(fnThrows)));

  static_assert(noexcept(transform_t::apply{}(std::declval<operand_t &>(), fnThrows))); // GAP #285

  SUCCEED();
}

TEST_CASE("transform choice", "[transform][choice]")
{
  using namespace fn;

  using operand_t = fn::choice<bool, double, int>;
  using operand_other_t = fn::choice<Xint>;
  using is = monadic_static_check<transform_t, operand_t>;

  constexpr auto fnValue = [](auto i) -> int { return i + 1; };
  constexpr auto fnXabs = [](int i) -> Xint { return Xint{std::abs(8 - i)}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](int) -> operand_t { throw 0; }));                   // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> operand_t { throw 0; }));              // allow conversion
  static_assert(is::invocable_with_any([](int) -> operand_other_t { throw 0; }));             // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> operand_t { throw 0; }));           // binds to const ref
  static_assert(is::invocable<lvalue>([](auto &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](auto &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::invocable<rvalue, crvalue>([](auto const &&) -> operand_t { throw 0; })); // binds to const rvalue

  constexpr auto fnLvalue = fn::overload{[](bool &) -> operand_t { throw 0; },   //
                                         [](double &) -> operand_t { throw 0; }, //
                                         [](int &) -> operand_t { throw 0; }};
  static_assert(is::not_invocable<clvalue, crvalue, cvalue>(fnLvalue)); // cannot remove const
  static_assert(is::not_invocable<rvalue>(fnLvalue));                   // disallow bind

  constexpr auto fnRvalue = fn::overload{[](bool &&) -> operand_t { throw 0; },   //
                                         [](double &&) -> operand_t { throw 0; }, //
                                         [](int &&) -> operand_t { throw 0; }};
  static_assert(is::not_invocable<lvalue, clvalue, crvalue, cvalue>(fnRvalue));      // cannot move
  static_assert(is::not_invocable_with_any([](int &) -> operand_t { throw 0; }));    // not enough types
  static_assert(is::not_invocable_with_any([]() -> operand_t { throw 0; }));         // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> operand_t { throw 0; })); // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | transform(fnValue));
      static_assert(std::is_same_v<T, fn::choice<int>>);
      REQUIRE(*(a | transform(fnValue)).get_ptr<int>() == 13);

      WHEN("change type")
      {
        using T = decltype(a | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::choice<Xint>>);
        REQUIRE((a | transform(fnXabs)).get_ptr<Xint>()->value == 4);
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | transform(fnValue));
      static_assert(std::is_same_v<T, fn::choice<int>>);
      REQUIRE(*(operand_t{12} | transform(fnValue)).get_ptr<int>() == 13);

      WHEN("change type")
      {
        using T = decltype(operand_t{12} | transform(fnXabs));
        static_assert(std::is_same_v<T, fn::choice<Xint>>);
        REQUIRE((operand_t{12} | transform(fnXabs)).get_ptr<Xint>()->value == 4);
      }
    }
  }
}

TEST_CASE("transform noexcept", "[transform][choice][noexcept]")
{
  using namespace fn;

  using operand_t = fn::choice<bool, double, int>;

  constexpr auto fnThrows = [](auto i) noexcept(false) -> int { return static_cast<int>(i) + 1; };

  // GAP #280: choice parts company with optional and expected here - its own transform is
  // unconditionally noexcept, dispatching through sum::transform, so even the MEMBER over-promises.
  // The same operation therefore has different exception behaviour depending on the monad it is
  // written against.
  static_assert(noexcept(std::declval<operand_t &>().transform(fnThrows)));

  static_assert(noexcept(transform_t::apply{}(std::declval<operand_t &>(), fnThrows))); // GAP #285

  SUCCEED();
}

TEST_CASE("constexpr transform expected", "[transform][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, Error>;

  WHEN("same value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> int {
      if (i < 2)
        return i + 1;
      return i;
    };
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(r1.value() == 1);
    constexpr auto r2 = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.value() == 2);
    constexpr auto r3 = T{::fn::unexpect, Error::SomethingElse} | fn::transform(fn);
    static_assert(r3.error() == Error::SomethingElse);
  }

  WHEN("different value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> bool { return (i == 1); };
    constexpr auto r1 = T{1} | fn::transform(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<bool, Error> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::transform(fn);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::transform(fn);
    static_assert(r3.value() == false);
    constexpr auto r4 = T{::fn::unexpect, Error::SomethingElse} | fn::transform(fn);
    static_assert(r4.error() == Error::SomethingElse);
  }

  SUCCEED();
}

TEST_CASE("constexpr transform expected with sum", "[transform][constexpr][expected][sum]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<fn::sum_for<Xint, int>, Error>;

  WHEN("same value type")
  {
    constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> fn::sum_for<Xint, int> {
                                       if (i < 3)
                                         return {i + 1};
                                       return i;
                                     },
                                     [](Xint v) constexpr noexcept -> fn::sum_for<Xint, int> { return v.value; }};
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum_for<Xint, int>, Error> const>);
    static_assert(r1.value() == fn::sum{1});
    constexpr auto r2 = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.value() == fn::sum{3});
    constexpr auto r3 = T{Xint{4}} | fn::transform(fn);
    static_assert(r3.value() == fn::sum{4});
    constexpr auto r4 = T{::fn::unexpect, Error::SomethingElse} | fn::transform(fn);
    static_assert(r4.error() == Error::SomethingElse);
  }

  WHEN("different value type")
  {
    constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> bool { return i == 1; },
                                     [](Xint v) constexpr noexcept -> int { return v.value; }};
    constexpr auto r1 = T{1} | fn::transform(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<fn::sum<bool, int>, Error> const>);
    static_assert(r1.value() == fn::sum{true});
    constexpr auto r2 = T{0} | fn::transform(fn);
    static_assert(r2.value() == fn::sum{false});
    constexpr auto r3 = T{Xint{3}} | fn::transform(fn);
    static_assert(r3.value() == fn::sum{3});
    constexpr auto r4 = T{::fn::unexpect, Error::SomethingElse} | fn::transform(fn);
    static_assert(r4.error() == Error::SomethingElse);
  }

  SUCCEED();
}

TEST_CASE("constexpr transform optional", "[transform][constexpr][optional]")
{
  using T = fn::optional<int>;

  WHEN("same value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> int {
      if (i < 2)
        return i + 1;
      return i;
    };
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(r1.value() == 1);
    constexpr auto r2 = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.value() == 2);
    constexpr auto r4 = T{} | fn::transform(fn);
    static_assert(not r4.has_value());
  }

  WHEN("different value type")
  {
    constexpr auto fn1 = [](int i) constexpr noexcept -> bool {
      if (i == 1)
        return true;
      return false;
    };
    constexpr auto r1 = T{1} | fn::transform(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::optional<bool> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::transform(fn1);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::transform(fn1);
    static_assert(r3.value() == false);
    constexpr auto r4 = T{} | fn::transform(fn1);
    static_assert(not r4.has_value());
  }

  SUCCEED();
}

TEST_CASE("constexpr transform optional with sum", "[transform][constexpr][optional][sum]")
{
  using T = fn::optional<fn::sum_for<Xint, int>>;

  WHEN("same value type")
  {
    constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> fn::sum_for<Xint, int> {
                                       if (i < 3)
                                         return {i + 1};
                                       return i;
                                     },
                                     [](Xint v) constexpr noexcept -> fn::sum_for<Xint, int> { return v.value; }};
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(std::is_same_v<decltype(r1), fn::optional<fn::sum_for<Xint, int>> const>);
    static_assert(r1.value() == fn::sum{1});
    constexpr auto r2 = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.value() == fn::sum{3});
    constexpr auto r3 = T{Xint{5}} | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r3.value() == fn::sum{5});
    constexpr auto r4 = T{} | fn::transform(fn);
    static_assert(not r4.has_value());
  }

  WHEN("different value type")
  {
    constexpr auto fn1 = fn::overload{[](int i) constexpr noexcept -> bool { return i == 1; },
                                      [](Xint v) constexpr noexcept -> int { return v.value; }};
    constexpr auto r1 = T{1} | fn::transform(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::optional<fn::sum<bool, int>> const>);
    static_assert(r1.value() == fn::sum{true});
    constexpr auto r2 = T{0} | fn::transform(fn1);
    static_assert(r2.value() == fn::sum{false});
    constexpr auto r3 = T{2} | fn::transform(fn1);
    static_assert(r3.value() == fn::sum{false});
    constexpr auto r4 = T{Xint{5}} | fn::transform(fn1);
    static_assert(r4.value() == fn::sum{5});
    constexpr auto r5 = T{} | fn::transform(fn1);
    static_assert(not r5.has_value());
  }

  SUCCEED();
}

TEST_CASE("constexpr transform choice", "[transform][constexpr][choice]")
{
  using T = fn::choice<double, int>;

  WHEN("same value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> int {
      if (i < 1)
        return i + 1;
      return i;
    };
    constexpr auto r1 = T{0} | fn::transform(fn);
    static_assert(r1.transform([](int i) -> int { return i; }) == fn::choice{1});
    constexpr auto r2 = T{0.5} | fn::transform(fn);
    static_assert(r2.transform([](int i) -> int { return i; }) == fn::choice{1});
    constexpr auto r3 = r1 | fn::transform(fn) | fn::transform(fn) | fn::transform(fn);
    static_assert(r2.transform([](int i) -> int { return i; }) == fn::choice{1});
  }

  WHEN("different value type")
  {
    constexpr auto fn1 = [](int i) constexpr noexcept -> bool { return (i == 1); };
    constexpr auto r1 = T{1} | fn::transform(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::choice<bool> const>);
    static_assert(r1.transform([](bool i) -> bool { return i; }) == fn::choice{true});
    constexpr auto r2 = T{0} | fn::transform(fn1);
    static_assert(r2.transform([](bool i) -> bool { return i; }) == fn::choice{false});
    constexpr auto r3 = T{2} | fn::transform(fn1);
    static_assert(r3.transform([](bool i) -> bool { return i; }) == fn::choice{false});
  }

  SUCCEED();
}
