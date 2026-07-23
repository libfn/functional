// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/static_check.hpp"

#include <fn/and_then.hpp>
#include <fn/functor.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <tuple>
#include <utility>

#include <fn/detail/macro_begin.hpp>

using namespace util;

namespace {
struct Error final {
  std::string what;
};
struct OtherError final {};

// C++26 to_string formats through std::format (P2587): to_string(0.5) is "0.5", not "0.500000"
#if defined(__cpp_lib_to_string) && __cpp_lib_to_string >= 202306L
constexpr char const got_84_and_half[] = "Got 84 and 0.5";
#else
constexpr char const got_84_and_half[] = "Got 84 and 0.500000";
#endif

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
  static_assert(is::template applicable<lvalue>(M::fn1));
  static_assert(is::template not_invocable<prvalue, cvalue, clvalue, rvalue, crvalue>(M::fn1));
  static_assert(is::invocable_with_any(M::fn2));
  static_assert(is::template applicable<prvalue, rvalue>(M::fn3));
  static_assert(is::template not_invocable<cvalue, lvalue, clvalue, crvalue>(M::fn3));
  static_assert(is::template applicable<prvalue, cvalue, rvalue, crvalue>(M::fn4));
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

      auto const r = fn::apply(and_then_t::apply{}, v, M::fn0);
      CHECK(r.value() == 2);

      auto const q = v | and_then(M::fn0);
      CHECK(q.value() == 2);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template not_invocable<lvalue>(M::fn1));

    SECTION("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(M::fn2));

      auto const r = fn::apply(and_then_t::apply{}, v, M::fn2);
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
          monadic_static_check<fn::and_then_t, decltype(v)>::template applicable<prvalue, crvalue, cvalue>(M::fn4));

      auto const r = fn::apply(and_then_t::apply{}, std::move(v), M::fn4);
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

      auto const r = fn::apply(and_then_t::apply{}, v, M::fn0);
      CHECK(r.value() == 2);

      auto const q = v | and_then(M::fn0);
      CHECK(q.value() == 2);
    }

    SECTION("lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template applicable<lvalue>(M::fn1));

      auto const r = fn::apply(and_then_t::apply{}, v, M::fn1);
      CHECK(r.value() == 3);

      auto const q = v | and_then(M::fn1);
      CHECK(q.value() == 3);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 3);
    }

    SECTION("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(M::fn2));

      auto const r = fn::apply(and_then_t::apply{}, v, M::fn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(M::fn2);
      CHECK(q.value() == 4);

      auto const s = std::as_const(v) | and_then(fn);
      CHECK(s.value() == 4);
    }

    SECTION("rvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::template applicable<prvalue, rvalue>(M::fn3));

      auto const r = fn::apply(and_then_t::apply{}, std::move(v), M::fn3);
      CHECK(r.value() == 5);

      auto const q = std::move(v) | and_then(M::fn3);
      CHECK(q.value() == 5);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 5);
    }

    SECTION("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::template applicable<prvalue, crvalue, cvalue>(M::fn4));

      auto const r = fn::apply(and_then_t::apply{}, std::move(v), M::fn4);
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
  constexpr auto fnFail = [](int i) -> operand_t { return ::fn::unexpected<Error>("Got " + std::to_string(i)); };
  constexpr auto fnXabs = [](int i) -> fn::expected<Xint, Error> { return {{std::abs(8 - i)}}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));               // allow generic call
  static_assert(is::invocable_with_any([](int) -> operand_t { throw 0; }));                   // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> operand_t { throw 0; }));              // allow conversion
  static_assert(is::invocable_with_any([](int) -> operand_other_t { throw 0; }));             // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> operand_t { throw 0; }));           // binds to const ref
  static_assert(is::applicable<lvalue>([](int &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::applicable<rvalue, prvalue>([](int &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::applicable<rvalue, crvalue>([](int const &&) -> operand_t { throw 0; })); // binds to const rvalue
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

    // and the verb carries that answer through: every step of the pipeline - the nielbloid,
    // operator|, _swap_invoke and apply - weighs what it dispatches to, so the pipe promises exactly
    // what the member does
    static_assert(noexcept(std::declval<nothrow_t &>() | and_then(fnNothrow2)));
    static_assert(not noexcept(std::declval<nothrow_t &>() | and_then(fnThrows2)));
    static_assert(not noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(not noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));

    // Constructing the functor copies the callable into a pack, and that copy can throw too.
    struct ThrowingCopy final {
      ThrowingCopy() = default;
      ThrowingCopy(ThrowingCopy const &) noexcept(false) {}
      ThrowingCopy(ThrowingCopy &&) noexcept(false) {}
      auto operator()(int i) const noexcept -> operand_t { return {i + 1}; }
    };
    static_assert(not std::is_nothrow_copy_constructible_v<ThrowingCopy>);
    static_assert(not noexcept(and_then(std::declval<ThrowingCopy const &>())));

    // fn::apply reaches the very same apply and reports the same answer: the library's two entry
    // points to one operation agree, where they once disagreed in opposite directions
    static_assert(not noexcept(fn::apply(and_then_t::apply{}, std::declval<operand_t &>(), fnThrows)));
    static_assert(noexcept(fn::apply(and_then_t::apply{}, std::declval<nothrow_t &>(), fnNothrow2)));

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
      operand_t a{::fn::unexpect, "Not good"};
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
            return ::fn::unexpected<Error>("Got " + std::to_string(i) + " and " + std::to_string(d));
          };
          using T = decltype(a | and_then(fnFail));
          static_assert(std::is_same_v<T, fn::expected<int, Error>>);
          REQUIRE((a | and_then(fnFail)).error().what == got_84_and_half);
        }
      }

      SECTION("error")
      {
        constexpr auto wrong = [](auto...) -> operand_t { throw 0; };
        REQUIRE((operand_t{::fn::unexpect, "Not good"} | and_then(wrong)).error().what == "Not good");
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
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, "Not good"} //
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

    SECTION("copack")
    {
      enum class Error { ThresholdExceeded, SomethingElse, UnexpectedType };
      using T = fn::expected<fn::copack_for<Xint, int>, Error>;

      SECTION("same value type")
      {
        constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T {
                                           if (i < 3)
                                             return {i + 1};
                                           return ::fn::unexpected<Error>{Error::ThresholdExceeded};
                                         },
                                         [](Xint v) constexpr noexcept -> T { return v; }};
        constexpr auto r1 = T{0} | fn::and_then(fn);
        static_assert(r1.value() == fn::copack{1});
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
      using T = fn::expected<int, fn::copack<Error>>;

      SECTION("same error type")
      {
        constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
          if (i < 2)
            return {i + 1};
          return ::fn::unexpected<int>{i};
        };

        constexpr auto r1 = T{0} | fn::and_then(fn1);
        static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::copack_for<Error, int>> const>);
        static_assert(r1.value() == 1);
        constexpr auto r2 = r1 | fn::and_then(fn1);
        static_assert(r2.value() == 2);
        constexpr auto r3 = r2 | fn::and_then(fn1);
        static_assert(r3.error() == fn::copack{2});
        constexpr auto r4 = r3 | fn::and_then(fn1);
        static_assert(r4.error() == fn::copack{2});

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
        static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::copack<Error>> const>);
        static_assert(r2.value());
        constexpr auto r3 = T{2} | fn::and_then(fn2);
        static_assert(r3.error() == fn::copack{Error::InvalidValue});

        constexpr auto fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
        constexpr auto r4 = r3 | fn::and_then(fn3);
        static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::copack_for<Error, int>> const>);
        static_assert(r4.error() == fn::copack{Error::InvalidValue});
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
  auto fnFail = [&count]() -> operand_t { return ::fn::unexpected<Error>("Got " + std::to_string(++count)); };
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

    // and the verb carries it through.
    static_assert(not noexcept(std::declval<nothrow_t &>() | and_then(fnThrows2)));
    static_assert(not noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(not noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));
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
      operand_t a{::fn::unexpect, "Not good"};
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
      using T = decltype(operand_t{::fn::unexpect, "Not good"} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{::fn::unexpect, "Not good"} //
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
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));               // allow generic call
  static_assert(is::invocable_with_any([](int) -> operand_t { throw 0; }));                   // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> operand_t { throw 0; }));              // allow conversion
  static_assert(is::invocable_with_any([](int) -> operand_other_t { throw 0; }));             // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> operand_t { throw 0; }));           // binds to const ref
  static_assert(is::applicable<lvalue>([](int &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::applicable<rvalue, prvalue>([](int &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::applicable<rvalue, crvalue>([](int const &&) -> operand_t { throw 0; })); // binds to const rvalue
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

    // As for expected: the member is precise, and the verb propagates it.
    static_assert(noexcept(std::declval<operand_t &>().and_then(fnNothrow)));
    static_assert(not noexcept(std::declval<operand_t &>().and_then(fnThrows)));
    static_assert(not noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(not noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));
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

    SECTION("copack")
    {
      using T = fn::optional<fn::copack_for<Xint, int>>;

      SECTION("same value type")
      {
        constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T {
                                           if (i < 3)
                                             return {i + 1};
                                           return {};
                                         },
                                         [](Xint v) constexpr noexcept -> T { return v; }};
        constexpr auto r1 = T{0} | fn::and_then(fn);
        static_assert(r1.value() == fn::copack{1});
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
  static_assert(is::invocable_with_any([](int) -> operand_t { throw 0; }));                    // allow copy
  static_assert(is::invocable_with_any([](unsigned) -> operand_t { throw 0; }));               // allow conversion
  static_assert(is::invocable_with_any([](int) -> operand_other_t { throw 0; }));              // allow conversion
  static_assert(is::invocable_with_any([](int const &) -> operand_t { throw 0; }));            // binds to const ref
  static_assert(is::applicable<lvalue>([](auto &) -> operand_t { throw 0; }));                 // binds to lvalue
  static_assert(is::applicable<rvalue, prvalue>([](auto &&) -> operand_t { throw 0; }));       // can move
  static_assert(is::applicable<rvalue, crvalue>([](auto const &&) -> operand_t { throw 0; })); // binds to const rvalue

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

    // the member propagates, as optional's and expected's do
    static_assert(not noexcept(std::declval<operand_t &>().and_then(fnThrows)));

    // and the verb layer propagates it, for every monad, choice included.
    static_assert(not noexcept(std::declval<operand_t &>() | and_then(fnThrows)));
    static_assert(not noexcept(std::declval<operand_t &&>() | and_then(fnThrows)));
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
      static_assert(r1.apply([](int i) -> int { return i; }) == 1);
      constexpr auto r2 = T{0.5} | fn::and_then(fn);
      static_assert(r2.apply([](int i) -> int { return i; }) == 1);
      constexpr auto r3 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
      static_assert(r3.apply([](double i) -> int { return i; }) == 0.0);

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
      static_assert(r1.apply([](bool i) -> bool { return i; }) == true);
      constexpr auto r2 = T{0} | fn::and_then(fn);
      static_assert(r2.apply([](bool i) -> bool { return i; }) == false);
      constexpr auto r3 = T{2} | fn::and_then(fn);
      static_assert(r3.apply([](int i) -> int { return i; }) == 2);

      SUCCEED();
    }
  }
}

TEST_CASE("and_then across the identity cluster", "[and_then][just][choice][expected][identity]")
{
  struct A final {
    bool operator==(A const &) const = default;
  };
  struct B final {
    bool operator==(B const &) const = default;
  };
  struct U final {
    bool operator==(U const &) const = default;
  };
  struct V final {
    bool operator==(V const &) const = default;
  };
  using E0 = fn::copack<>;

  SECTION("endo binds stay on the members")
  {
    // the piped form of the superset join (the member's), value-returning here to pin the type
    auto r1 = fn::choice_for<A, B>{A{}}
              | fn::and_then(fn::overload{[](A) { return fn::choice<U>{U{}}; }, //
                                          [](B) { return fn::choice_for<U, V>{V{}}; }});
    static_assert(std::is_same_v<decltype(r1), fn::choice_for<U, V>>);
    CHECK(r1 == fn::choice_for<U, V>{U{}});
    auto r2 = fn::just{3} | fn::and_then([](int i) { return fn::just<bool>{i != 0}; });
    static_assert(std::is_same_v<decltype(r2), fn::just<bool>>);
    CHECK(r2.value());
    // an identity expected keeps member semantics for expected results, widening included
    auto r3 = fn::expected<int, E0>{42} | fn::and_then([](int) { return fn::expected<bool, U>{true}; });
    static_assert(std::is_same_v<decltype(r3), fn::expected<bool, fn::copack<U>>>);
    CHECK(r3.value());
  }

  SECTION("choice crosses to a convergent carrier")
  {
    constexpr auto fnJust = fn::overload{[](A) { return fn::just<int>{1}; }, //
                                         [](B) { return fn::just<int>{2}; }};
    auto r = fn::choice_for<A, B>{A{}} | fn::and_then(fnJust);
    static_assert(std::is_same_v<decltype(r), fn::just<int>>);
    CHECK(r.value() == 1);
    CHECK((fn::choice_for<A, B>{B{}} | fn::and_then(fnJust)).value() == 2);
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    // (the workaround's full story is in tests/fn/choice.cpp)
    constexpr fn::choice_for<A, B> ca{A{}};
    static_assert((ca | fn::and_then(fnJust)).value() == 1);
  }

  SECTION("just crosses kinds")
  {
    auto r1 = fn::just{3} | fn::and_then([](int) { return fn::choice<U>{U{}}; });
    static_assert(std::is_same_v<decltype(r1), fn::choice<U>>);
    CHECK(r1 == fn::choice<U>{U{}});
    auto r2 = fn::just{3} | fn::and_then([](int i) { return fn::expected<bool, E0>{i != 0}; });
    static_assert(std::is_same_v<decltype(r2), fn::expected<bool, E0>>);
    CHECK(r2.value());
    static_assert((fn::just{3} | fn::and_then([](int) { return fn::choice<U>{U{}}; })) == fn::choice<U>{U{}});
  }

  SECTION("identity expected evaporates into the superset choice")
  {
    constexpr auto fnJoin = fn::overload{[](A) { return fn::choice<U>{U{}}; }, //
                                         [](B) { return fn::choice_for<U, V>{V{}}; }};
    fn::expected<fn::copack_for<A, B>, E0> e{fn::copack_for<A, B>{B{}}};
    auto r = std::move(e) | fn::and_then(fnJoin);
    static_assert(std::is_same_v<decltype(r), fn::choice_for<U, V>>);
    CHECK(r == fn::choice_for<U, V>{V{}});
    // named source: the same VS 2022 misread as above
    constexpr fn::expected<fn::copack_for<A, B>, E0> ea{fn::copack_for<A, B>{A{}}};
    static_assert((ea | fn::and_then(fnJoin)) == fn::choice_for<U, V>{U{}});
  }

  SECTION("identity expected, single and void payloads; the void just")
  {
    auto r1 = fn::expected<int, E0>{42} | fn::and_then([](int i) { return fn::just<int>{i}; });
    static_assert(std::is_same_v<decltype(r1), fn::just<int>>);
    CHECK(r1.value() == 42);
    auto r2 = fn::expected<void, E0>{} | fn::and_then([] { return fn::choice<U>{U{}}; });
    static_assert(std::is_same_v<decltype(r2), fn::choice<U>>);
    CHECK(r2 == fn::choice<U>{U{}});
    auto r3 = fn::just{} | fn::and_then([] { return fn::just<int>{9}; });
    static_assert(std::is_same_v<decltype(r3), fn::just<int>>);
    CHECK(r3.value() == 9);
  }

  SECTION("noexcept and constraints")
  {
    constexpr auto nothrowCross = [](int) noexcept { return fn::choice<U>{U{}}; };
    fn::just<int> j{3};
    static_assert(noexcept(j | fn::and_then(nothrowCross)));
    static_assert(not noexcept(j | fn::and_then([](int) { return fn::choice<U>{U{}}; })));

    // only an identity input crosses kinds, and only to an identity result
    static_assert(fn::applicable_and_then_across<decltype(nothrowCross), fn::just<int> &>);
    constexpr auto fnJust = [](int) { return fn::just<int>{1}; };
    static_assert(not fn::applicable_and_then_across<decltype(fnJust), fn::expected<int, U> &>);
    static_assert(not fn::applicable_and_then_across<decltype(fnJust), fn::optional<int> &>);
    constexpr auto fnValue = [](int i) { return i; };
    static_assert(not fn::applicable_and_then_across<decltype(fnValue), fn::just<int> &>);
    // an endo callback is the member's business, not the cluster's
    static_assert(not fn::applicable_and_then_across<decltype(fnJust), fn::just<int> &>);
    // an uninhabited copack payload has no branches to join - the probe answers, not asserts
    static_assert(not fn::applicable_and_then_across<decltype(fnJust), fn::expected<fn::copack<>, E0> &>);
    SUCCEED();
  }
}

TEST_CASE("and_then joins heterogeneous expected branches", "[and_then][expected][copack]")
{
  struct A final {
    bool operator==(A const &) const = default;
  };
  struct B final {
    bool operator==(B const &) const = default;
  };
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
  using In = fn::expected<fn::copack_for<A, B>, fn::copack<E0>>;
  using InB = fn::expected<fn::copack_for<A, B>, fn::copack<>>;
  constexpr auto fnJoin = fn::overload{[](A) { return fn::expected<X, fn::copack<E1>>{X{}}; },
                                       [](B) { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
  constexpr auto canM = [](auto &&v, auto &&fn) { return requires { FWD(v).and_then(FWD(fn)); }; };
  constexpr auto canP = [](auto &&v, auto &&fn) { return requires { FWD(v) | fn::and_then(FWD(fn)); }; };

  SECTION("values join, errors union with self's grade")
  {
    auto r = In{fn::copack_for<A, B>{B{}}}.and_then(fnJoin);
    static_assert(std::is_same_v<decltype(r), fn::expected<fn::copack_for<X, Y>, fn::copack_for<E0, E1, E2>>>);
    CHECK(r.value() == fn::copack_for<X, Y>{Y{}});
    auto e = In{fn::unexpect, fn::copack<E0>{E0{}}}.and_then(fnJoin);
    CHECK(e.error() == fn::copack_for<E0, E1, E2>{E0{}});
    // the error path in every remaining value category
    In el{fn::unexpect, fn::copack<E0>{E0{}}};
    CHECK(el.and_then(fnJoin).error() == fn::copack_for<E0, E1, E2>{E0{}});
    CHECK(std::as_const(el).and_then(fnJoin).error() == fn::copack_for<E0, E1, E2>{E0{}});
    CHECK(std::move(std::as_const(el)).and_then(fnJoin).error() == fn::copack_for<E0, E1, E2>{E0{}});
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    constexpr In ca{fn::copack_for<A, B>{A{}}};
    static_assert(ca.and_then(fnJoin).value() == fn::copack_for<X, Y>{X{}});

    // the piped spelling agrees
    auto p = In{fn::copack_for<A, B>{A{}}} | fn::and_then(fnJoin);
    static_assert(std::is_same_v<decltype(p), fn::expected<fn::copack_for<X, Y>, fn::copack_for<E0, E1, E2>>>);
    CHECK(p.value() == fn::copack_for<X, Y>{X{}});
  }

  SECTION("a plain error is retained exactly while the values still join")
  {
    using InP = fn::expected<fn::copack_for<A, B>, E0>;
    constexpr auto fn2 = fn::overload{[](A) { return fn::expected<X, E0>{X{}}; }, //
                                      [](B) { return fn::expected<Y, E0>{Y{}}; }};
    auto r = InP{fn::copack_for<A, B>{A{}}}.and_then(fn2);
    static_assert(std::is_same_v<decltype(r), fn::expected<fn::copack_for<X, Y>, E0>>);
    CHECK(r.value() == fn::copack_for<X, Y>{X{}});
    // ... and a branch changing the plain error answers, not errors
    constexpr auto fnBad = fn::overload{[](A) { return fn::expected<X, E0>{X{}}; }, //
                                        [](B) { return fn::expected<Y, E1>{Y{}}; }};
    static_assert(not canM(InP{fn::copack_for<A, B>{A{}}}, fnBad));
  }

  SECTION("copack-valued results flatten and overlapping alternatives dedup")
  {
    constexpr auto fn3 = fn::overload{
        [](A) { return fn::expected<fn::copack_for<X, Y>, fn::copack_for<E0, E1>>{fn::copack_for<X, Y>{X{}}}; },
        [](B) { return fn::expected<Y, fn::copack<E1>>{Y{}}; }};
    auto r = In{fn::copack_for<A, B>{B{}}}.and_then(fn3);
    static_assert(std::is_same_v<decltype(r), fn::expected<fn::copack_for<X, Y>, fn::copack_for<E0, E1>>>);
    CHECK(r.value() == fn::copack_for<X, Y>{Y{}});
  }

  SECTION("a copack<> grade acquires the branch errors")
  {
    auto r = InB{fn::copack_for<A, B>{A{}}}.and_then(fnJoin);
    static_assert(std::is_same_v<decltype(r), fn::expected<fn::copack_for<X, Y>, fn::copack_for<E1, E2>>>);
    CHECK(r.value() == fn::copack_for<X, Y>{X{}});

    // the piped spelling agrees - an identity input must not divert the join into the cluster
    auto p = InB{fn::copack_for<A, B>{B{}}} | fn::and_then(fnJoin);
    static_assert(std::is_same_v<decltype(p), fn::expected<fn::copack_for<X, Y>, fn::copack_for<E1, E2>>>);
    CHECK(p.value() == fn::copack_for<X, Y>{Y{}});
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    constexpr InB cb{fn::copack_for<A, B>{A{}}};
    static_assert((cb | fn::and_then(fnJoin)).value() == fn::copack_for<X, Y>{X{}});
  }

  SECTION("convergent branches keep their exact type and the widening behaviour")
  {
    constexpr auto fnConv = fn::overload{[](A) { return fn::expected<X, fn::copack<E1>>{X{}}; },
                                         [](B) { return fn::expected<X, fn::copack<E1>>{X{}}; }};
    auto r = In{fn::copack_for<A, B>{A{}}}.and_then(fnConv);
    static_assert(std::is_same_v<decltype(r), fn::expected<X, fn::copack_for<E0, E1>>>);
    CHECK(r.value() == X{});

    // branches convergent only after stripping cv/ref engage the join instead: the hetero tier
    // already owns reference-returning branches, so a more-alike set must not assert
    // (no constexpr twin - the reference-returning branch needs static storage, barred in
    // constant evaluation until C++23)
    constexpr auto fnRefConv = fn::overload{[](A) -> fn::expected<X, fn::copack<E1>> & {
                                              static fn::expected<X, fn::copack<E1>> e{X{}};
                                              return e;
                                            },
                                            [](B) { return fn::expected<X, fn::copack<E1>>{X{}}; }};
    auto rr = In{fn::copack_for<A, B>{A{}}}.and_then(fnRefConv);
    static_assert(std::is_same_v<decltype(rr), fn::expected<X, fn::copack_for<E0, E1>>>);
    CHECK(rr.value() == X{});
    CHECK((In{fn::copack_for<A, B>{B{}}} | fn::and_then(fnRefConv)).value() == X{}); // the pipe agrees
  }

  SECTION("all-void branches join to void; mixed void and non-void answers")
  {
    constexpr auto fnVoid = fn::overload{[](A) { return fn::expected<void, fn::copack<E1>>{}; },
                                         [](B) { return fn::expected<void, fn::copack<E2>>{}; }};
    auto r = In{fn::copack_for<A, B>{A{}}}.and_then(fnVoid);
    static_assert(std::is_same_v<decltype(r), fn::expected<void, fn::copack_for<E0, E1, E2>>>);
    CHECK(r.has_value());
    constexpr auto fnMixed = fn::overload{[](A) { return fn::expected<void, fn::copack<E1>>{}; },
                                          [](B) { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
    static_assert(not canM(In{fn::copack_for<A, B>{A{}}}, fnMixed));
    static_assert(not fn::applicable_and_then<decltype(fnMixed), In>);
    static_assert(fn::applicable_and_then<decltype(fnJoin), In>); // converse
  }

  SECTION("noexcept from the reachable constructions")
  {
    In v{fn::copack_for<A, B>{A{}}};
    constexpr auto fnNothrow = fn::overload{[](A) noexcept { return fn::expected<X, fn::copack<E1>>{X{}}; },
                                            [](B) noexcept { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
    static_assert(noexcept(v.and_then(fnNothrow)));
    constexpr auto fnThrows = fn::overload{[](A) { return fn::expected<X, fn::copack<E1>>{X{}}; },
                                           [](B) noexcept { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
    static_assert(not noexcept(v.and_then(fnThrows)));

    // the widening arms weigh in: nothrow branches whose result relocates a throwing-move
    // alternative into the joined value still make the join throwing
    struct ThrowingMove final {
      ThrowingMove() = default;
      ThrowingMove(ThrowingMove &&) noexcept(false) {}
      bool operator==(ThrowingMove const &) const = default;
    };
    constexpr auto fnThrowingArm
        = fn::overload{[](A) noexcept { return fn::expected<ThrowingMove, fn::copack<E1>>{std::in_place}; },
                       [](B) noexcept { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
    static_assert(not noexcept(v.and_then(fnThrowingArm)));
    // lifting self's error into the union weighs on the path that relocates it
    using InT = fn::expected<fn::copack_for<A, B>, fn::copack<ThrowingMove>>;
    static_assert(not noexcept(std::declval<InT &&>().and_then(fnNothrow)));
    // ... while a copack<> grade has no error to lift, and the dead arm cannot weigh
    InB b{fn::copack_for<A, B>{A{}}};
    static_assert(noexcept(b.and_then(fnNothrow)));
    SUCCEED();
  }

  SECTION("exceptions")
  {
    // the widening relocation may throw at runtime: the exception propagates, and self - whose
    // alternative the callback consumed by reference only - is left unchanged
    struct Boom final {
      int fuse; // the fuse-th relocation throws
      constexpr explicit Boom(int f) noexcept : fuse(f) {}
      constexpr Boom(Boom &&o) noexcept(false) : fuse(o.fuse - 1)
      {
        if (fuse == 0)
          throw 0;
      }
    };
    constexpr auto fnBoom = fn::overload{[](A) { return fn::expected<Boom, fn::copack<E1>>{Boom{2}}; },
                                         [](B) { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
    In self{fn::copack_for<A, B>{A{}}};
    CHECK_THROWS_AS(self.and_then(fnBoom), int);
    CHECK(self.value() == fn::copack_for<A, B>{A{}});

    // the same relocation completing
    constexpr auto fnSafe = fn::overload{[](A) { return fn::expected<Boom, fn::copack<E1>>{Boom{99}}; },
                                         [](B) { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
    auto r = In{fn::copack_for<A, B>{A{}}}.and_then(fnSafe);
    CHECK(r.value().has_value(std::in_place_type<Boom>));
    static_assert([] {
      constexpr auto fnSafeX = fn::overload{[](A) { return fn::expected<Boom, fn::copack<E1>>{Boom{99}}; },
                                            [](B) { return fn::expected<Y, fn::copack<E2>>{Y{}}; }};
      return In{fn::copack_for<A, B>{A{}}}.and_then(fnSafeX).value().has_value(std::in_place_type<Boom>);
    }());
  }

  SECTION("a plain grade lifts into its singular copack")
  {
    // the convergent path: the callback's error side spells copack<E> over a plain-E self
    using InL = fn::expected<fn::copack_for<A, B>, E0>;
    constexpr auto fnLift = fn::overload{[](A) { return fn::expected<X, fn::copack<E0>>{X{}}; },
                                         [](B) { return fn::expected<X, fn::copack<E0>>{X{}}; }};
    auto r = InL{fn::copack_for<A, B>{A{}}}.and_then(fnLift);
    static_assert(std::is_same_v<decltype(r), fn::expected<X, fn::copack<E0>>>);
    CHECK(r.value() == X{});
    // ... and the heterogeneous path: one branch grades, the plain branches lift with it - the
    // copack spelling wins, so grading never silently drops
    constexpr auto fnMix = fn::overload{[](A) { return fn::expected<X, E0>{X{}}; },
                                        [](B) { return fn::expected<Y, fn::copack<E0>>{Y{}}; }};
    auto m = InL{fn::copack_for<A, B>{B{}}}.and_then(fnMix);
    static_assert(std::is_same_v<decltype(m), fn::expected<fn::copack_for<X, Y>, fn::copack<E0>>>);
    CHECK(m.value() == fn::copack_for<X, Y>{Y{}});
    auto e = InL{fn::unexpect, E0{}}.and_then(fnMix);
    CHECK(e.error() == fn::copack<E0>{E0{}});
    // a DIFFERENT plain error still refuses - the lift is the singular copack of self's own grade
    constexpr auto fnBad = fn::overload{[](A) { return fn::expected<X, E0>{X{}}; },
                                        [](B) { return fn::expected<Y, fn::copack<E1>>{Y{}}; }};
    static_assert(not canM(InL{fn::copack_for<A, B>{A{}}}, fnBad));

    // the piped spelling and the concept agree with the member
    auto p = InL{fn::copack_for<A, B>{B{}}} | fn::and_then(fnMix);
    static_assert(std::is_same_v<decltype(p), fn::expected<fn::copack_for<X, Y>, fn::copack<E0>>>);
    CHECK(p.value() == fn::copack_for<X, Y>{Y{}});
    static_assert(fn::applicable_and_then<decltype(fnMix), InL>);
    static_assert(fn::applicable_and_then<decltype(fnLift), InL>);
    static_assert(not fn::applicable_and_then<decltype(fnBad), InL>);
  }

  SECTION("the functor answers over an identity input")
  {
    // the cluster arm's probe must not claim the member's join, nor assert on what neither owns
    static_assert(not fn::applicable_and_then_across<decltype(fnJoin), InB>);
    constexpr auto fnRaw = fn::overload{[](A) { return 1; }, [](B) { return 2L; }};
    static_assert(not fn::applicable_and_then_across<decltype(fnRaw), InB>);
    static_assert(not canM(InB{fn::copack_for<A, B>{A{}}}, fnRaw));
    static_assert(not canP(InB{fn::copack_for<A, B>{A{}}}, fnRaw));
    static_assert(canP(InB{fn::copack_for<A, B>{A{}}}, fnJoin)); // converse
    // ... nor on branches convergent only after stripping cv/ref - select compares exact types
    constexpr auto fnRef = fn::overload{[](A) -> X & {
                                          static X x{};
                                          return x;
                                        },
                                        [](B) { return X{}; }};
    static_assert(not fn::applicable_and_then_across<decltype(fnRef), InB>);
    static_assert(not canP(InB{fn::copack_for<A, B>{A{}}}, fnRef));
    SUCCEED();
  }
}

TEST_CASE("and_then joins heterogeneous optional branches", "[and_then][optional][copack]")
{
  struct A final {
    bool operator==(A const &) const = default;
  };
  struct B final {
    bool operator==(B const &) const = default;
  };
  struct X final {
    bool operator==(X const &) const = default;
  };
  struct Y final {
    bool operator==(Y const &) const = default;
  };
  using In = fn::optional<fn::copack_for<A, B>>;
  constexpr auto fnJoin = fn::overload{[](A) { return fn::optional<X>{X{}}; }, //
                                       [](B) { return fn::optional<Y>{Y{}}; }};
  constexpr auto canM = [](auto &&v, auto &&fn) { return requires { FWD(v).and_then(FWD(fn)); }; };

  SECTION("the values join; nullopt passes through as the joined type")
  {
    auto r = In{fn::copack_for<A, B>{B{}}}.and_then(fnJoin);
    static_assert(std::is_same_v<decltype(r), fn::optional<fn::copack_for<X, Y>>>);
    CHECK(r.value() == fn::copack_for<X, Y>{Y{}});
    auto n = In{std::nullopt}.and_then(fnJoin);
    static_assert(std::is_same_v<decltype(n), fn::optional<fn::copack_for<X, Y>>>);
    CHECK(not n.has_value());
    // named source: VS 2022 misreads a mid-expression prvalue's empty-class union member
    constexpr In ca{fn::copack_for<A, B>{A{}}};
    static_assert(ca.and_then(fnJoin).value() == fn::copack_for<X, Y>{X{}});

    // the piped spelling agrees
    auto p = In{fn::copack_for<A, B>{A{}}} | fn::and_then(fnJoin);
    static_assert(std::is_same_v<decltype(p), fn::optional<fn::copack_for<X, Y>>>);
    CHECK(p.value() == fn::copack_for<X, Y>{X{}});
  }

  SECTION("convergent branches keep their exact type")
  {
    constexpr auto fnConv = fn::overload{[](A) { return fn::optional<X>{X{}}; }, //
                                         [](B) { return fn::optional<X>{X{}}; }};
    auto r = In{fn::copack_for<A, B>{A{}}}.and_then(fnConv);
    static_assert(std::is_same_v<decltype(r), fn::optional<X>>);
    CHECK(r.value() == X{});

    // branches convergent only after stripping cv/ref engage the join to the common type
    // (no constexpr twin - the reference-returning branch needs static storage, barred in
    // constant evaluation until C++23)
    constexpr auto fnRefConv = fn::overload{[](A) -> fn::optional<X> & {
                                              static fn::optional<X> o{X{}};
                                              return o;
                                            },
                                            [](B) { return fn::optional<X>{X{}}; }};
    auto rr = In{fn::copack_for<A, B>{A{}}}.and_then(fnRefConv);
    static_assert(std::is_same_v<decltype(rr), fn::optional<X>>);
    CHECK(rr.value() == X{});
  }

  SECTION("constraints and noexcept")
  {
    // a mixed optional-and-expected set answers, not errors
    constexpr auto fnMixed
        = fn::overload{[](A) { return fn::optional<X>{X{}}; }, [](B) { return fn::expected<Y, fn::copack<>>{Y{}}; }};
    static_assert(not canM(In{fn::copack_for<A, B>{A{}}}, fnMixed));
    static_assert(canM(In{fn::copack_for<A, B>{A{}}}, fnJoin)); // converse
    static_assert(fn::applicable_and_then<decltype(fnJoin), In>);
    static_assert(not fn::applicable_and_then<decltype(fnMixed), In>);
    // a reference-carrying optional leaves the join unformable rather than copying the referent
    constexpr auto fnRef = fn::overload{[](A) -> fn::optional<X &> { throw 0; }, //
                                        [](B) { return fn::optional<Y>{Y{}}; }};
    static_assert(not canM(In{fn::copack_for<A, B>{A{}}}, fnRef));

    In v{fn::copack_for<A, B>{A{}}};
    constexpr auto fnNothrow = fn::overload{[](A) noexcept { return fn::optional<X>{X{}}; },
                                            [](B) noexcept { return fn::optional<Y>{Y{}}; }};
    static_assert(noexcept(v.and_then(fnNothrow)));
    constexpr auto fnThrows
        = fn::overload{[](A) { return fn::optional<X>{X{}}; }, [](B) noexcept { return fn::optional<Y>{Y{}}; }};
    static_assert(not noexcept(v.and_then(fnThrows)));
    // the widening arms weigh in here too
    struct ThrowingMove final {
      ThrowingMove() = default;
      ThrowingMove(ThrowingMove &&) noexcept(false) {}
      bool operator==(ThrowingMove const &) const = default;
    };
    constexpr auto fnThrowingArm = fn::overload{[](A) noexcept { return fn::optional<ThrowingMove>{std::in_place}; },
                                                [](B) noexcept { return fn::optional<Y>{Y{}}; }};
    static_assert(not noexcept(v.and_then(fnThrowingArm)));
    SUCCEED();
  }

  SECTION("exceptions")
  {
    struct Boom final {
      int fuse; // the fuse-th relocation throws
      constexpr explicit Boom(int f) noexcept : fuse(f) {}
      constexpr Boom(Boom &&o) noexcept(false) : fuse(o.fuse - 1)
      {
        if (fuse == 0)
          throw 0;
      }
    };
    constexpr auto fnBoom = fn::overload{[](A) { return fn::optional<Boom>{Boom{2}}; }, //
                                         [](B) { return fn::optional<Y>{Y{}}; }};
    In self{fn::copack_for<A, B>{A{}}};
    CHECK_THROWS_AS(self.and_then(fnBoom), int);
    CHECK(self.value() == fn::copack_for<A, B>{A{}}); // self unchanged

    constexpr auto fnSafe = fn::overload{[](A) { return fn::optional<Boom>{Boom{99}}; }, //
                                         [](B) { return fn::optional<Y>{Y{}}; }};
    auto r = In{fn::copack_for<A, B>{A{}}}.and_then(fnSafe);
    CHECK(r.value().has_value(std::in_place_type<Boom>));
    static_assert([] {
      constexpr auto fnSafeX = fn::overload{[](A) { return fn::optional<Boom>{Boom{99}}; }, //
                                            [](B) { return fn::optional<Y>{Y{}}; }};
      return In{fn::copack_for<A, B>{A{}}}.and_then(fnSafeX).value().has_value(std::in_place_type<Boom>);
    }());
  }
}

TEST_CASE("and_then tuple-like payload", "[and_then][expected][optional][tuple]")
{
  // a lone tuple-like payload exposes its elements to the callback, member and functor alike
  struct Error final {
    bool operator==(Error const &) const = default;
  };
  using T = std::tuple<int, int>;
  constexpr auto fnAdd = [](int a, int b) noexcept { return fn::expected<int, Error>{a + b}; };

  fn::expected<T, Error> e{std::in_place, 20, 22};
  CHECK(e.and_then(fnAdd).value() == 42);
  CHECK((e | fn::and_then(fnAdd)).value() == 42);
  constexpr auto fnOpt = [](int a, int b) noexcept { return fn::optional<int>{a + b}; };
  fn::optional<T> o{std::in_place, 20, 22};
  CHECK(o.and_then(fnOpt).value() == 42);
  CHECK((o | fn::and_then(fnOpt)).value() == 42);

  SECTION("constexpr")
  {
    static_assert(fn::expected<T, Error>{std::in_place, 20, 22}.and_then(fnAdd).value() == 42);
    static_assert((fn::optional<T>{std::in_place, 20, 22} | fn::and_then(fnOpt)).value() == 42);
    SUCCEED();
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
static_assert(applicable_and_then<decltype(fn_int<expected<Value, Error>>), expected<int, Error>>);
static_assert(applicable_and_then<decltype(fn_int<expected<void, Error>>), expected<int, Error>>);
static_assert(not applicable_and_then<decltype(fn_int<expected<int, Xerror>>), expected<int, Error>>);           // different error_type
static_assert(not applicable_and_then<decltype(fn_int<expected<int, Error>>), expected<Value, Error>>);          // wrong parameter type
static_assert(applicable_and_then<decltype(fn_generic<expected<int, Error>>), expected<Value, Error>>);
static_assert(not applicable_and_then<decltype(fn_generic<expected<int, Xerror>>), expected<Value, Error>>);     // different error_type
static_assert(applicable_and_then<decltype(fn_generic<expected<int, Error>>), expected<void, Error>>);
static_assert(applicable_and_then<decltype(fn_generic<expected<void, Error>>), expected<void, Error>>);
static_assert(applicable_and_then<decltype(fn_generic<expected<Value, Error>>), expected<void, Error>>);
static_assert(not applicable_and_then<decltype(fn_generic<expected<int, Xerror>>), expected<void, Error>>);      // different error_type
static_assert(not applicable_and_then<decltype(fn_generic<expected<void, Xerror>>), expected<void, Error>>);     // different error_type
static_assert(not applicable_and_then<decltype(fn_generic<expected<int, Error>>), optional<Value>>);             // mixed optional and expected
static_assert(not applicable_and_then<decltype(fn_generic<expected<int, Xerror>>), optional<int>>);              // mixed optional and expected
static_assert(not applicable_and_then<decltype(fn_generic<optional<int>>), expected<Value, Error>>);             // mixed optional and expected
static_assert(applicable_and_then<decltype(fn_generic<optional<int>>), optional<Value>>);
static_assert(applicable_and_then<decltype(fn_generic<optional<Value>>), optional<int>>);
static_assert(not applicable_and_then<decltype(fn_int_lvalue<expected<Value, Error>>), expected<int, Error>>);   // cannot bind temporary to lvalue
static_assert(applicable_and_then<decltype(fn_int_lvalue<expected<Value, Error>>), expected<int, Error> &>);
static_assert(applicable_and_then<decltype(fn_int_rvalue<expected<Value, Error>>), expected<int, Error>>);
static_assert(not applicable_and_then<decltype(fn_int_rvalue<expected<Value, Error>>), expected<int, Error> &>); // cannot bind lvalue to rvalue-ref

// A copack error type grades the monad: the callback may then return an expected with a DIFFERENT error,
// which the operation widens into the copack. Without one, the error types must match exactly - the two
// disjuncts above and below are what tell those cases apart.
static_assert(applicable_and_then<decltype(fn_int<expected<Value, copack<Error>>>), expected<int, copack<Error>>>);
static_assert(applicable_and_then<decltype(fn_int<expected<Value, Xerror>>), expected<int, copack<Error>>>);         // widens the error
static_assert(applicable_and_then<decltype(fn_int<expected<Value, Error>>), expected<int, copack<Error, Xerror>>>);  // already covered by the copack
static_assert(not applicable_and_then<decltype(fn_int<expected<Value, Xerror>>), expected<int, Error>>);          // no copack: error must match
static_assert(applicable_and_then<decltype(fn_generic<expected<Value, Xerror>>), expected<void, copack<Error>>>);    // ... and the same for void
static_assert(not applicable_and_then<decltype(fn_generic<expected<Value, Xerror>>), expected<void, Error>>);

// choice - the concept's remaining disjunct
static_assert(applicable_and_then<decltype(fn_generic<choice<int>>), choice<int>>);
static_assert(applicable_and_then<decltype(fn_generic<choice<Value>>), choice<int>>);                            // may change the type
static_assert(not applicable_and_then<decltype(fn_generic<optional<int>>), choice<int>>);                        // must stay a choice
static_assert(not applicable_and_then<decltype(fn_generic<expected<int, Error>>), choice<int>>);
static_assert(not applicable_and_then<decltype(fn_generic<choice<int>>), optional<int>>);                        // mixed choice and optional
static_assert(not applicable_and_then<decltype(fn_int_lvalue<choice<int>>), choice<int>>);                       // cannot bind temporary to lvalue
static_assert(applicable_and_then<decltype(fn_int_lvalue<choice<int>>), choice<int> &>);
// clang-format on
} // namespace fn
