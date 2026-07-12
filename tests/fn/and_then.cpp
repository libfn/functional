// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/and_then.hpp>
#include <fn/functor.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <utility>

using namespace util;

namespace {
struct Error final {
  std::string what;
};
struct OtherError final {};

struct Xint final {
  int value;

  static auto efn(Xint const &self) noexcept -> fn::expected<int, Error> { return {self.value}; }
  auto efn1() & noexcept -> fn::expected<int, Error> { return {value + 1}; }
  auto efn2() const & noexcept -> fn::expected<int, Error> { return {value + 2}; }
  auto efn3() && noexcept -> fn::expected<int, Error> { return {value + 3}; }
  auto efn4() const && noexcept -> fn::expected<int, Error> { return {value + 4}; }

  static auto ofn(Xint const &self) noexcept -> fn::optional<int> { return {self.value}; }
  auto ofn1() & noexcept -> fn::optional<int> { return {value + 1}; }
  auto ofn2() const & noexcept -> fn::optional<int> { return {value + 2}; }
  auto ofn3() && noexcept -> fn::optional<int> { return {value + 3}; }
  auto ofn4() const && noexcept -> fn::optional<int> { return {value + 4}; }
};

template <typename R> struct Xfn final {
  auto operator()(Xint &v) const noexcept -> R { return {v.value + 1}; }
  auto operator()(Xint const &v) const noexcept -> R { return {v.value + 2}; }
  auto operator()(Xint &&v) const noexcept -> R { return {v.value + 3}; }
  auto operator()(Xint const &&v) const noexcept -> R { return {v.value + 4}; }
};

using ExpectedXint = fn::expected<Xint, Error>;
using OptionalXint = fn::optional<Xint>;

// One battery serves both monads: the member set differs only in the returned monad.
template <typename Operand> struct member_fns;
template <> struct member_fns<ExpectedXint> {
  using result_t = fn::expected<int, Error>;
  static constexpr auto fn0 = Xint::efn;
  static constexpr auto fn1 = &Xint::efn1;
  static constexpr auto fn2 = &Xint::efn2;
  static constexpr auto fn3 = &Xint::efn3;
  static constexpr auto fn4 = &Xint::efn4;
};
template <> struct member_fns<OptionalXint> {
  using result_t = fn::optional<int>;
  static constexpr auto fn0 = Xint::ofn;
  static constexpr auto fn1 = &Xint::ofn1;
  static constexpr auto fn2 = &Xint::ofn2;
  static constexpr auto fn3 = &Xint::ofn3;
  static constexpr auto fn4 = &Xint::ofn4;
};

template <typename Operand> constexpr bool member_viability()
{
  using is = monadic_static_check<fn::and_then_t, Operand>;
  using M = member_fns<Operand>;

  static_assert(is::invocable_with_any(M::fn0));
  static_assert(is::template invocable<lvalue>(M::fn1));
  static_assert(is::template not_invocable<prvalue, cvalue, clvalue, rvalue, crvalue>(M::fn1));
  static_assert(is::invocable_with_any(M::fn2));
  static_assert(is::template invocable<prvalue, rvalue>(M::fn3));
  static_assert(is::template not_invocable<cvalue, lvalue, clvalue, crvalue>(M::fn3));
  static_assert(is::template invocable<prvalue, cvalue, rvalue, crvalue>(M::fn4));
  static_assert(is::template not_invocable<lvalue, clvalue>(M::fn4));
  return true;
}
static_assert(member_viability<ExpectedXint>());
static_assert(member_viability<OptionalXint>());
} // namespace

TEMPLATE_TEST_CASE("and_then member", "[and_then][member_functions]", ExpectedXint, OptionalXint)
{
  using namespace fn;

  using M = member_fns<TestType>;
  constexpr Xfn<typename M::result_t> fn{};

  SECTION("const")
  {
    TestType const v{std::in_place, Xint{2}};

    SECTION("static fn")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(M::fn0));

      auto const r = fn::invoke(and_then_t::apply{}, v, M::fn0);
      CHECK(r.value() == 2);

      auto const q = v | and_then(M::fn0);
      CHECK(q.value() == 2);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template not_invocable<lvalue>(M::fn1));

    SECTION("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(M::fn2));

      auto const r = fn::invoke(and_then_t::apply{}, v, M::fn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(M::fn2);
      CHECK(q.value() == 4);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 4);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template not_invocable<rvalue>(M::fn3));

    SECTION("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::template invocable<prvalue, crvalue, cvalue>(M::fn4));

      auto const r = fn::invoke(and_then_t::apply{}, std::move(v), M::fn4);
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(M::fn4);
      CHECK(q.value() == 6);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  SECTION("mutable")
  {
    TestType v{std::in_place, Xint{2}};

    SECTION("static fn")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(M::fn0));

      auto const r = fn::invoke(and_then_t::apply{}, v, M::fn0);
      CHECK(r.value() == 2);

      auto const q = v | and_then(M::fn0);
      CHECK(q.value() == 2);
    }

    SECTION("lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template invocable<lvalue>(M::fn1));

      auto const r = fn::invoke(and_then_t::apply{}, v, M::fn1);
      CHECK(r.value() == 3);

      auto const q = v | and_then(M::fn1);
      CHECK(q.value() == 3);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 3);
    }

    SECTION("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(M::fn2));

      auto const r = fn::invoke(and_then_t::apply{}, v, M::fn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(M::fn2);
      CHECK(q.value() == 4);

      auto const s = std::as_const(v) | and_then(fn);
      CHECK(s.value() == 4);
    }

    SECTION("rvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template invocable<prvalue, rvalue>(M::fn3));

      auto const r = fn::invoke(and_then_t::apply{}, std::move(v), M::fn3);
      CHECK(r.value() == 5);

      auto const q = std::move(v) | and_then(M::fn3);
      CHECK(q.value() == 5);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 5);
    }

    SECTION("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::template invocable<prvalue, crvalue, cvalue>(M::fn4));

      auto const r = fn::invoke(and_then_t::apply{}, std::move(v), M::fn4);
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(M::fn4);
      CHECK(q.value() == 6);

      auto const s = std::move(std::as_const(v)) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }
}

TEST_CASE("and_then", "[and_then][expected][expected_value][pack]")
{
  using namespace fn;

  using operand_t = fn::expected<int, Error>;
  using operand_other_t = fn::expected<void, Error>;
  using operand_other_err_t = fn::expected<int, OtherError>;
  using is = monadic_static_check<and_then_t, operand_t>;

  constexpr auto fnValue = [](int i) -> operand_t { return {i + 1}; };
  constexpr auto wrong = [](int) -> operand_t { throw 0; };
  constexpr auto fnFail = [](int i) -> operand_t { return ::fn::unexpected<Error>(Error{"Got " + std::to_string(i)}); };
  constexpr auto fnXabs = [](int i) -> fn::expected<Xint, Error> { return {{std::abs(8 - i)}}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));              // allow generic call
  static_assert(is::invocable_with_any([](int) -> operand_t { throw 0; }));                  // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> operand_t { throw 0; }));             // allow conversion
  static_assert(is::invocable_with_any([](int) -> operand_other_t { throw 0; }));            // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> operand_t { throw 0; }));          // binds to const ref
  static_assert(is::invocable<lvalue>([](int &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](int &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::invocable<rvalue, crvalue>([](int const &&) -> operand_t { throw 0; })); // binds to const rvalue
  static_assert(is::not_invocable_with_any([](int) -> operand_other_err_t { throw 0; })); // disallow error conversion
  static_assert(
      is::not_invocable<clvalue, crvalue, cvalue>([](int &) -> operand_t { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](int &) -> operand_t { throw 0; }));         // disallow bind
  static_assert(
      is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) -> operand_t { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> operand_t { throw 0; }));           // bad type
  static_assert(is::not_invocable_with_any([]() -> operand_t { throw 0; }));                      // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> operand_t { throw 0; }));              // bad arity

  SECTION("noexcept")
  {
    constexpr auto fnNothrow = [](int i) noexcept -> operand_t { return {i + 1}; };
    constexpr auto fnThrows = [](int i) noexcept(false) -> operand_t { return {i + 1}; };

    // The member's spec is precise, and weighs BOTH sides: the callback, and the copy of the
    // untouched error. Error carries a std::string, whose copy can throw on its own - so even a
    // noexcept callback leaves the member potentially-throwing here.
    static_assert(not std::is_nothrow_copy_constructible_v<Error>);
    static_assert(not noexcept(std::declval<operand_t &>().and_then(fnNothrow)));
    static_assert(not noexcept(std::declval<operand_t &>().and_then(fnThrows)));

    // Give it an error whose copy cannot throw, and the callback alone decides.
    using nothrow_t = fn::expected<int, int>;
    constexpr auto fnNothrow2 = [](int i) noexcept -> nothrow_t { return {i + 1}; };
    constexpr auto fnThrows2 = [](int i) noexcept(false) -> nothrow_t { return {i + 1}; };
    static_assert(noexcept(std::declval<nothrow_t &>().and_then(fnNothrow2)));
    static_assert(not noexcept(std::declval<nothrow_t &>().and_then(fnThrows2)));

    // GAP #285: the verb then discards that answer. Every step of the pipeline - the nielbloid,
    // operator|, _swap_invoke and apply - is unconditionally noexcept, so a throwing callback the
    // member would propagate instead crosses a noexcept boundary and terminates.
    static_assert(noexcept(std::declval<nothrow_t &>() | and_then(fnThrows2)));
    static_assert(noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));

    // Constructing the functor copies the callable into a pack, and that copy can throw too.
    struct ThrowingCopy final {
      ThrowingCopy() = default;
      ThrowingCopy(ThrowingCopy const &) noexcept(false) {}
      ThrowingCopy(ThrowingCopy &&) noexcept(false) {}
      auto operator()(int i) const noexcept -> operand_t { return {i + 1}; }
    };
    static_assert(not std::is_nothrow_copy_constructible_v<ThrowingCopy>);
    static_assert(noexcept(and_then(std::declval<ThrowingCopy const &>()))); // GAP #285

    // fn::invoke reaches the very same apply, yet reports noexcept(false) even for a callback that
    // cannot throw - the #45 traits behind it are stubbed false. The library's two entry points to
    // one operation disagree, and each is wrong in the opposite direction.
    static_assert(not noexcept(fn::invoke(and_then_t::apply{}, std::declval<operand_t &>(), fnNothrow)));

    SUCCEED();
  }

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | and_then(fnValue)).value() == 13);

      SECTION("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | and_then(fnFail)).error().what == "Got 12");
      }

      SECTION("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((a | and_then(fnXabs)).value().value == 4);
      }
    }

    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }

    SECTION("pack")
    {
      using operand_t = fn::expected<fn::pack<int, double>, Error>;
      operand_t a{std::in_place, fn::pack{84, 0.5}};
      constexpr auto fnPack = [](int i, double d) constexpr -> fn::expected<int, Error> { return {i * d}; };
      using T = decltype(a | and_then(fnPack));
      static_assert(std::is_same_v<T, fn::expected<int, Error>>);

      SECTION("value")
      {
        REQUIRE((a | and_then(fnPack)).value() == 42);
        SECTION("fail")
        {
          constexpr auto fnFail = [](int i, double d) constexpr -> fn::expected<int, Error> {
            return ::fn::unexpected<Error>(Error{"Got " + std::to_string(i) + " and " + std::to_string(d)});
          };
          using T = decltype(a | and_then(fnFail));
          static_assert(std::is_same_v<T, fn::expected<int, Error>>);
          REQUIRE((a | and_then(fnFail)).error().what == "Got 84 and 0.500000");
        }
      }

      SECTION("error")
      {
        constexpr auto wrong = [](auto...) -> operand_t { throw 0; };
        REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} | and_then(wrong)).error().what == "Not good");
      }
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place, 12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | and_then(fnValue)).value() == 13);

      SECTION("fail")
      {
        using T = decltype(operand_t{std::in_place, 12} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 12} | and_then(fnFail)).error().what == "Got 12");
      }

      SECTION("change type")
      {
        using T = decltype(operand_t{std::in_place, 12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place, 12} | and_then(fnXabs)).value().value == 4);
      }
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  SECTION("constexpr")
  {
    enum class Error { ThresholdExceeded, SomethingElse };
    using T = fn::expected<int, Error>;

    SECTION("same value type")
    {
      constexpr auto fn = [](int i) constexpr noexcept -> T {
        if (i < 3)
          return {i + 1};
        return ::fn::unexpected<Error>{Error::ThresholdExceeded};
      };
      constexpr auto r1 = T{0} | fn::and_then(fn);
      static_assert(r1.value() == 1);
      constexpr auto r2 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
      static_assert(r2.error() == Error::ThresholdExceeded);

      SUCCEED();
    }

    SECTION("different value type")
    {
      using T1 = fn::expected<bool, Error>;
      constexpr auto fn = [](int i) constexpr noexcept -> T1 {
        if (i == 1)
          return {true};
        if (i == 0)
          return {false};
        return ::fn::unexpected<Error>{Error::SomethingElse};
      };
      constexpr auto r1 = T{1} | fn::and_then(fn);
      static_assert(std::is_same_v<decltype(r1), fn::expected<bool, Error> const>);
      static_assert(r1.value() == true);
      constexpr auto r2 = T{0} | fn::and_then(fn);
      static_assert(r2.value() == false);
      constexpr auto r3 = T{2} | fn::and_then(fn);
      static_assert(r3.error() == Error::SomethingElse);

      SUCCEED();
    }

    SECTION("sum")
    {
      enum class Error { ThresholdExceeded, SomethingElse, UnexpectedType };
      using T = fn::expected<fn::sum_for<Xint, int>, Error>;

      SECTION("same value type")
      {
        constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T {
                                           if (i < 3)
                                             return {i + 1};
                                           return ::fn::unexpected<Error>{Error::ThresholdExceeded};
                                         },
                                         [](Xint v) constexpr noexcept -> T { return v; }};
        constexpr auto r1 = T{0} | fn::and_then(fn);
        static_assert(r1.value() == fn::sum{1});
        constexpr auto r2 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
        static_assert(r2.error() == Error::ThresholdExceeded);

        SUCCEED();
      }

      SECTION("different value type")
      {
        using T1 = fn::expected<bool, Error>;
        constexpr auto fn = fn::overload{
            [](int i) constexpr noexcept -> T1 {
              if (i == 1)
                return {true};
              if (i == 0)
                return {false};
              return ::fn::unexpected<Error>{Error::SomethingElse};
            },
            [](Xint) constexpr noexcept -> T1 { return ::fn::unexpected<Error>{Error::UnexpectedType}; }};
        constexpr auto r1 = T{1} | fn::and_then(fn);
        static_assert(std::is_same_v<decltype(r1), fn::expected<bool, Error> const>);
        static_assert(r1.value() == true);
        constexpr auto r2 = T{0} | fn::and_then(fn);
        static_assert(r2.value() == false);
        constexpr auto r3 = T{2} | fn::and_then(fn);
        static_assert(r3.error() == Error::SomethingElse);

        SUCCEED();
      }
    }

    SECTION("graded")
    {
      enum class Error { InvalidValue };
      using T = fn::expected<int, fn::sum<Error>>;

      SECTION("same error type")
      {
        constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
          if (i < 2)
            return {i + 1};
          return ::fn::unexpected<int>{i};
        };

        constexpr auto r1 = T{0} | fn::and_then(fn1);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum_for<Error, int>> const>);
        static_assert(r1.value() == 1);
        constexpr auto r2 = r1 | fn::and_then(fn1);
        static_assert(r2.value() == 2);
        constexpr auto r3 = r2 | fn::and_then(fn1);
        static_assert(r3.error() == fn::sum{2});
        constexpr auto r4 = r3 | fn::and_then(fn1);
        static_assert(r4.error() == fn::sum{2});

        SUCCEED();
      }

      SECTION("accumulate errors")
      {
        constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
          if (i < 0 || i > 1)
            return ::fn::unexpected<Error>{Error::InvalidValue};
          return {i == 1};
        };

        constexpr auto r2 = T{1} | fn::and_then(fn2);
        static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::sum<Error>> const>);
        static_assert(r2.value());
        constexpr auto r3 = T{2} | fn::and_then(fn2);
        static_assert(r3.error() == fn::sum{Error::InvalidValue});

        constexpr auto fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
        constexpr auto r4 = r3 | fn::and_then(fn3);
        static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::sum_for<Error, int>> const>);
        static_assert(r4.error() == fn::sum{Error::InvalidValue});
        constexpr auto r5 = T{2} | fn::and_then(fn3);
        static_assert(r5.value() == 3);

        SUCCEED();
      }
    }
  }
}

TEST_CASE("and_then", "[and_then][expected][expected_void]")
{
  using namespace fn;

  using operand_t = fn::expected<void, Error>;
  using operand_other_t = fn::expected<int, Error>;
  using operand_other_err_t = fn::expected<void, OtherError>;
  using is = monadic_static_check<and_then_t, operand_t>;

  int count = 0;
  auto fnValue = [&count]() -> operand_t {
    count += 1;
    return {};
  };

  constexpr auto wrong = []() -> operand_t { throw 0; };
  auto fnFail = [&count]() -> operand_t { return ::fn::unexpected<Error>(Error{"Got " + std::to_string(++count)}); };
  auto fnXabs = [&count]() -> fn::expected<Xint, Error> { return {{++count}}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));        // allow generic call
  static_assert(is::invocable_with_any([]() -> operand_other_t { throw 0; }));         // allow conversion
  static_assert(is::not_invocable_with_any([]() -> operand_other_err_t { throw 0; })); // disallow error conversion
  static_assert(is::not_invocable_with_any([](int) -> operand_t { throw 0; }));        // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> operand_t { throw 0; }));   // bad arity

  SECTION("noexcept")
  {
    constexpr auto fnNothrow = []() noexcept -> operand_t { return {}; };
    constexpr auto fnThrows = []() noexcept(false) -> operand_t { return {}; };

    // As for a non-void value: the member weighs the untouched error's copy too, and Error's can
    // throw - so give it a nothrow error to see the callback alone decide.
    static_assert(not noexcept(std::declval<operand_t &>().and_then(fnNothrow)));

    using nothrow_t = fn::expected<void, int>;
    constexpr auto fnNothrow2 = []() noexcept -> nothrow_t { return {}; };
    constexpr auto fnThrows2 = []() noexcept(false) -> nothrow_t { return {}; };
    static_assert(noexcept(std::declval<nothrow_t &>().and_then(fnNothrow2)));
    static_assert(not noexcept(std::declval<nothrow_t &>().and_then(fnThrows2)));

    // GAP #285: the verb is unconditionally noexcept regardless.
    static_assert(noexcept(std::declval<nothrow_t &>() | and_then(fnThrows2)));
    static_assert(noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));
    SUCCEED();
  }

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (a | and_then(fnValue)).value();
      CHECK(count == 1);

      SECTION("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | and_then(fnFail)).error().what == "Got 2");
      }

      SECTION("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((a | and_then(fnXabs)).value().value == 2);
      }
    }
    SECTION("error")
    {
      operand_t a{::fn::unexpect, Error{"Not good"}};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{std::in_place} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | and_then(fnValue)).value();
      CHECK(count == 1);

      SECTION("fail")
      {
        using T = decltype(operand_t{std::in_place} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place} | and_then(fnFail)).error().what == "Got 2");
      }

      SECTION("change type")
      {
        using T = decltype(operand_t{std::in_place} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place} | and_then(fnXabs)).value().value == 2);
      }
    }
    SECTION("error")
    {
      using T = decltype(operand_t{::fn::unexpect, Error{"Not good"}} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, Error{"Not good"}} //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }
}

TEST_CASE("and_then", "[and_then][optional][pack]")
{
  using namespace fn;

  using operand_t = fn::optional<int>;
  using operand_other_t = fn::optional<double>;
  using is = monadic_static_check<and_then_t, operand_t>;

  constexpr auto fnValue = [](int i) -> operand_t { return {i + 1}; };
  constexpr auto wrong = [](int) -> operand_t { throw 0; };
  constexpr auto fnFail = [](int) -> operand_t { return {}; };
  constexpr auto fnXabs = [](int i) -> fn::optional<Xint> { return {{std::abs(8 - i)}}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));              // allow generic call
  static_assert(is::invocable_with_any([](int) -> operand_t { throw 0; }));                  // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> operand_t { throw 0; }));             // allow conversion
  static_assert(is::invocable_with_any([](int) -> operand_other_t { throw 0; }));            // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> operand_t { throw 0; }));          // binds to const ref
  static_assert(is::invocable<lvalue>([](int &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::invocable<rvalue, prvalue>([](int &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::invocable<rvalue, crvalue>([](int const &&) -> operand_t { throw 0; })); // binds to const rvalue
  static_assert(
      is::not_invocable<clvalue, crvalue, cvalue>([](int &) -> operand_t { throw 0; })); // cannot remove const
  static_assert(is::not_invocable<rvalue>([](int &) -> operand_t { throw 0; }));         // disallow bind
  static_assert(
      is::not_invocable<lvalue, clvalue, crvalue, cvalue>([](int &&) -> operand_t { throw 0; })); // cannot move
  static_assert(is::not_invocable_with_any([](std::string) -> operand_t { throw 0; }));           // bad type
  static_assert(is::not_invocable_with_any([]() -> operand_t { throw 0; }));                      // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> operand_t { throw 0; }));              // bad arity

  SECTION("noexcept")
  {
    constexpr auto fnNothrow = [](int i) noexcept -> operand_t { return {i + 1}; };
    constexpr auto fnThrows = [](int i) noexcept(false) -> operand_t { return {i + 1}; };

    // As for expected: the member is precise, the verb is not (GAP #285).
    static_assert(noexcept(std::declval<operand_t &>().and_then(fnNothrow)));
    static_assert(not noexcept(std::declval<operand_t &>().and_then(fnThrows)));
    static_assert(noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));
    SUCCEED();
  }

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | and_then(fnValue)).value() == 13);

      SECTION("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | and_then(fnFail)).has_value());
      }

      SECTION("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::optional<Xint>>);
        REQUIRE((a | and_then(fnXabs)).value().value == 4);
      }
    }
    SECTION("error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | and_then(wrong)).has_value());
    }
  }

  SECTION("pack")
  {
    using operand_t = fn::optional<fn::pack<int, double>>;
    operand_t a{std::in_place, fn::pack{84, 0.5}};
    constexpr auto fnPack = [](int i, double d) constexpr -> fn::optional<int> { return {i * d}; };
    using T = decltype(a | and_then(fnPack));
    static_assert(std::is_same_v<T, fn::optional<int>>);

    SECTION("value")
    {
      REQUIRE((a | and_then(fnPack)).value() == 42);

      SECTION("fail")
      {
        constexpr auto fnFail = [](int, double) constexpr -> fn::optional<int> { return {std::nullopt}; };
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, fn::optional<int>>);
        REQUIRE(not(a | and_then(fnFail)).has_value());
      }
    }

    SECTION("error")
    {
      constexpr auto wrong = [](auto...) -> operand_t { throw 0; };
      REQUIRE(not(operand_t{std::nullopt} | and_then(wrong)).has_value());
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | and_then(fnValue)).value() == 13);

      SECTION("fail")
      {
        using T = decltype(operand_t{12} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{12} | and_then(fnFail)).has_value());
      }

      SECTION("change type")
      {
        using T = decltype(operand_t{12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::optional<Xint>>);
        REQUIRE((operand_t{12} | and_then(fnXabs)).value().value == 4);
      }
    }
    SECTION("error")
    {
      using T = decltype(operand_t{std::nullopt} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | and_then(wrong))
                     .has_value());
    }
  }

  SECTION("constexpr")
  {
    using T = fn::optional<int>;

    SECTION("same value type")
    {
      constexpr auto fn = [](int i) constexpr noexcept -> T {
        if (i < 3)
          return {i + 1};
        return {};
      };
      constexpr auto r1 = T{0} | fn::and_then(fn);
      static_assert(r1.value() == 1);
      constexpr auto r2 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
      static_assert(not r2.has_value());

      SUCCEED();
    }

    SECTION("different value type")
    {
      using T1 = fn::optional<bool>;
      constexpr auto fn = [](int i) constexpr noexcept -> T1 {
        if (i == 1)
          return {true};
        if (i == 0)
          return {false};
        return {};
      };
      constexpr auto r1 = T{1} | fn::and_then(fn);
      static_assert(std::is_same_v<decltype(r1), fn::optional<bool> const>);
      static_assert(r1.value() == true);
      constexpr auto r2 = T{0} | fn::and_then(fn);
      static_assert(r2.value() == false);
      constexpr auto r3 = T{2} | fn::and_then(fn);
      static_assert(not r3.has_value());

      SUCCEED();
    }

    SECTION("sum")
    {
      using T = fn::optional<fn::sum_for<Xint, int>>;

      SECTION("same value type")
      {
        constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T {
                                           if (i < 3)
                                             return {i + 1};
                                           return {};
                                         },
                                         [](Xint v) constexpr noexcept -> T { return v; }};
        constexpr auto r1 = T{0} | fn::and_then(fn);
        static_assert(r1.value() == fn::sum{1});
        constexpr auto r2 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
        static_assert(not r2.has_value());

        SUCCEED();
      }

      SECTION("different value type")
      {
        using T1 = fn::optional<bool>;
        constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T1 {
                                           if (i == 1)
                                             return {true};
                                           if (i == 0)
                                             return {false};
                                           return {};
                                         },
                                         [](Xint) constexpr noexcept -> T1 { return {}; }};
        constexpr auto r1 = T{1} | fn::and_then(fn);
        static_assert(std::is_same_v<decltype(r1), fn::optional<bool> const>);
        static_assert(r1.value() == true);
        constexpr auto r2 = T{0} | fn::and_then(fn);
        static_assert(r2.value() == false);
        constexpr auto r3 = T{2} | fn::and_then(fn);
        static_assert(not r3.has_value());

        SUCCEED();
      }
    }
  }
}

TEST_CASE("and_then choice", "[and_then][choice]")
{
  using namespace fn;

  using operand_t = fn::choice<bool, double, int>;
  using operand_other_t = fn::choice<Xint>;
  using is = monadic_static_check<and_then_t, operand_t>;

  constexpr auto fnValue = [](auto i) -> operand_t { return {i + 1}; };
  constexpr auto fnXabs = [](int i) -> operand_other_t { return {Xint{std::abs(8 - i)}}; };

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

  SECTION("noexcept")
  {
    constexpr auto fnThrows = [](auto i) noexcept(false) -> operand_t { return {i + 1}; };

    // GAP #280: choice differs from its siblings - its own and_then is unconditionally noexcept too
    // (it dispatches through sum::invoke), so here even the MEMBER over-promises. The same monadic
    // operation therefore has different exception behaviour depending on which monad it is written
    // against: optional and expected propagate, choice terminates.
    static_assert(noexcept(std::declval<operand_t &>().and_then(fnThrows)));

    // GAP #285: the verb layer over-promises for every monad, choice included.
    static_assert(noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));
    SUCCEED();
  }

  SECTION("lvalue")
  {
    SECTION("value")
    {
      operand_t a{12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(*(a | and_then(fnValue)).get_ptr<int>() == 13);

      SECTION("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::choice<Xint>>);
        REQUIRE((a | and_then(fnXabs)).get_ptr<Xint>()->value == 4);
      }
    }
  }

  SECTION("rvalue")
  {
    SECTION("value")
    {
      using T = decltype(operand_t{12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(*(operand_t{12} | and_then(fnValue)).get_ptr<int>() == 13);

      SECTION("change type")
      {
        using T = decltype(operand_t{12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::choice<Xint>>);
        REQUIRE((operand_t{12} | and_then(fnXabs)).get_ptr<Xint>()->value == 4);
      }
    }
  }

  SECTION("constexpr")
  {
    using T = fn::choice<double, int>;

    SECTION("same value type")
    {
      constexpr auto fn = [](int i) constexpr noexcept -> T {
        if (i < 3)
          return {i + 1};
        return {0.0};
      };
      constexpr auto r1 = T{0} | fn::and_then(fn);
      static_assert(r1.invoke([](int i) -> int { return i; }) == 1);
      constexpr auto r2 = T{0.5} | fn::and_then(fn);
      static_assert(r2.invoke([](int i) -> int { return i; }) == 1);
      constexpr auto r3 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
      static_assert(r3.invoke([](double i) -> int { return i; }) == 0.0);

      SUCCEED();
    }

    SECTION("different value type")
    {
      using T1 = fn::choice<bool, int>;
      constexpr auto fn = [](int i) constexpr noexcept -> T1 {
        if (i == 1)
          return {true};
        else if (i == 0)
          return {false};
        else
          return {std::move(i)};
      };
      constexpr auto r1 = T{1} | fn::and_then(fn);
      static_assert(std::is_same_v<decltype(r1), fn::choice<bool, int> const>);
      static_assert(r1.invoke([](bool i) -> bool { return i; }) == true);
      constexpr auto r2 = T{0} | fn::and_then(fn);
      static_assert(r2.invoke([](bool i) -> bool { return i; }) == false);
      constexpr auto r3 = T{2} | fn::and_then(fn);
      static_assert(r3.invoke([](int i) -> int { return i; }) == 2);

      SUCCEED();
    }
  }
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename T> constexpr auto fn_int = [](int) -> T { throw 0; };
template <typename T> constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };
template <typename T> constexpr auto fn_int_lvalue = [](int &) -> T { throw 0; };
template <typename T> [[maybe_unused]] constexpr auto fn_int_const_lvalue = [](int const &) -> T { throw 0; };
template <typename T> constexpr auto fn_int_rvalue = [](int &&) -> T { throw 0; };
template <typename T> [[maybe_unused]] constexpr auto fn_int_const_rvalue = [](int const &&) -> T { throw 0; };

} // namespace

// clang-format off
static_assert(invocable_and_then<decltype(fn_int<expected<Value, Error>>), expected<int, Error>>);
static_assert(invocable_and_then<decltype(fn_int<expected<void, Error>>), expected<int, Error>>);
static_assert(not invocable_and_then<decltype(fn_int<expected<int, Xerror>>), expected<int, Error>>);           // different error_type
static_assert(not invocable_and_then<decltype(fn_int<expected<int, Error>>), expected<Value, Error>>);          // wrong parameter type
static_assert(invocable_and_then<decltype(fn_generic<expected<int, Error>>), expected<Value, Error>>);
static_assert(not invocable_and_then<decltype(fn_generic<expected<int, Xerror>>), expected<Value, Error>>);     // different error_type
static_assert(invocable_and_then<decltype(fn_generic<expected<int, Error>>), expected<void, Error>>);
static_assert(invocable_and_then<decltype(fn_generic<expected<void, Error>>), expected<void, Error>>);
static_assert(invocable_and_then<decltype(fn_generic<expected<Value, Error>>), expected<void, Error>>);
static_assert(not invocable_and_then<decltype(fn_generic<expected<int, Xerror>>), expected<void, Error>>);      // different error_type
static_assert(not invocable_and_then<decltype(fn_generic<expected<void, Xerror>>), expected<void, Error>>);     // different error_type
static_assert(not invocable_and_then<decltype(fn_generic<expected<int, Error>>), optional<Value>>);             // mixed optional and expected
static_assert(not invocable_and_then<decltype(fn_generic<expected<int, Xerror>>), optional<int>>);              // mixed optional and expected
static_assert(not invocable_and_then<decltype(fn_generic<optional<int>>), expected<Value, Error>>);             // mixed optional and expected
static_assert(invocable_and_then<decltype(fn_generic<optional<int>>), optional<Value>>);
static_assert(invocable_and_then<decltype(fn_generic<optional<Value>>), optional<int>>);
static_assert(not invocable_and_then<decltype(fn_int_lvalue<expected<Value, Error>>), expected<int, Error>>);   // cannot bind temporary to lvalue
static_assert(invocable_and_then<decltype(fn_int_lvalue<expected<Value, Error>>), expected<int, Error> &>);
static_assert(invocable_and_then<decltype(fn_int_rvalue<expected<Value, Error>>), expected<int, Error>>);
static_assert(not invocable_and_then<decltype(fn_int_rvalue<expected<Value, Error>>), expected<int, Error> &>); // cannot bind lvalue to rvalue-ref

// A sum error type grades the monad: the callback may then return an expected with a DIFFERENT error,
// which the operation widens into the sum. Without one, the error types must match exactly - the two
// disjuncts above and below are what tell those cases apart.
static_assert(invocable_and_then<decltype(fn_int<expected<Value, sum<Error>>>), expected<int, sum<Error>>>);
static_assert(invocable_and_then<decltype(fn_int<expected<Value, Xerror>>), expected<int, sum<Error>>>);         // widens the error
static_assert(invocable_and_then<decltype(fn_int<expected<Value, Error>>), expected<int, sum<Error, Xerror>>>);  // already covered by the sum
static_assert(not invocable_and_then<decltype(fn_int<expected<Value, Xerror>>), expected<int, Error>>);          // no sum: error must match
static_assert(invocable_and_then<decltype(fn_generic<expected<Value, Xerror>>), expected<void, sum<Error>>>);    // ... and the same for void
static_assert(not invocable_and_then<decltype(fn_generic<expected<Value, Xerror>>), expected<void, Error>>);

// choice - the concept's remaining disjunct
static_assert(invocable_and_then<decltype(fn_generic<choice<int>>), choice<int>>);
static_assert(invocable_and_then<decltype(fn_generic<choice<Value>>), choice<int>>);                            // may change the type
static_assert(not invocable_and_then<decltype(fn_generic<optional<int>>), choice<int>>);                        // must stay a choice
static_assert(not invocable_and_then<decltype(fn_generic<expected<int, Error>>), choice<int>>);
static_assert(not invocable_and_then<decltype(fn_generic<choice<int>>), optional<int>>);                        // mixed choice and optional
static_assert(not invocable_and_then<decltype(fn_int_lvalue<choice<int>>), choice<int>>);                       // cannot bind temporary to lvalue
static_assert(invocable_and_then<decltype(fn_int_lvalue<choice<int>>), choice<int> &>);
// clang-format on
} // namespace fn
