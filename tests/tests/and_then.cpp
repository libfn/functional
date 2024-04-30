// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/and_then.hpp"
#include "functional/functor.hpp"

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

namespace check_expected {
using operand_t = fn::expected<Xint, Error>;
using is = monadic_static_check<fn::and_then_t, operand_t>;

static_assert(is::invocable_with_any(Xint::efn));
static_assert(is::invocable<lvalue>(&Xint::efn1));
static_assert(is::not_invocable<prvalue, cvalue, clvalue, rvalue, crvalue>(&Xint::efn1));
static_assert(is::invocable_with_any(&Xint::efn2));
static_assert(is::invocable<prvalue, rvalue>(&Xint::efn3));
static_assert(is::not_invocable<cvalue, lvalue, clvalue, crvalue>(&Xint::efn3));
static_assert(is::invocable<prvalue, cvalue, rvalue, crvalue>(&Xint::efn4));
static_assert(is::not_invocable<lvalue, clvalue>(&Xint::efn4));
} // namespace check_expected

namespace check_optional {
using operand_t = fn::optional<Xint>;
using is = monadic_static_check<fn::and_then_t, operand_t>;

static_assert(is::invocable_with_any(Xint::ofn));
static_assert(is::invocable<lvalue>(&Xint::ofn1));
static_assert(is::not_invocable<prvalue, cvalue, clvalue, rvalue, crvalue>(&Xint::ofn1));
static_assert(is::invocable_with_any(&Xint::ofn2));
static_assert(is::invocable<prvalue, rvalue>(&Xint::ofn3));
static_assert(is::not_invocable<cvalue, lvalue, clvalue, crvalue>(&Xint::ofn3));
static_assert(is::invocable<prvalue, cvalue, rvalue, crvalue>(&Xint::ofn4));
static_assert(is::not_invocable<lvalue, clvalue>(&Xint::ofn4));
} // namespace check_optional
} // namespace

TEST_CASE("and_then_member", "[and_then][member_functions]")
{
  using namespace fn;

  WHEN("expected const")
  {
    constexpr Xfn<fn::expected<int, Error>> fn{};
    fn::expected<Xint, Error> const v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(Xint::efn));

      auto const r = fn::invoke(and_then_t::apply{}, Xint::efn, v);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::efn);
      CHECK(q.value() == 2);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::not_invocable<lvalue>(&Xint::efn1));

    WHEN("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(&Xint::efn2));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::efn2, v);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::efn2);
      CHECK(q.value() == 4);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 4);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::not_invocable<rvalue>(&Xint::efn3));

    WHEN("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::invocable<prvalue, crvalue, cvalue>(&Xint::efn4));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::efn4, std::move(v));
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::efn4);
      CHECK(q.value() == 6);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  WHEN("expected mutable")
  {
    constexpr Xfn<fn::expected<int, Error>> fn{};
    fn::expected<Xint, Error> v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(Xint::efn));

      auto const r = fn::invoke(and_then_t::apply{}, Xint::efn, v);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::efn);
      CHECK(q.value() == 2);
    }

    WHEN("lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable<lvalue>(&Xint::efn1));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::efn1, v);
      CHECK(r.value() == 3);

      auto const q = v | and_then(&Xint::efn1);
      CHECK(q.value() == 3);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 3);
    }

    WHEN("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(&Xint::efn2));

      // rvalue-ref
      auto const r = fn::invoke(and_then_t::apply{}, &Xint::efn2, v);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::efn2);
      CHECK(q.value() == 4);

      auto const s = std::as_const(v) | and_then(fn);
      CHECK(s.value() == 4);
    }

    WHEN("rvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable<prvalue, rvalue>(&Xint::efn3));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::efn3, std::move(v));
      CHECK(r.value() == 5);

      auto const q = std::move(v) | and_then(&Xint::efn3);
      CHECK(q.value() == 5);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 5);
    }

    WHEN("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::invocable<prvalue, crvalue, cvalue>(&Xint::efn4));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::efn4, std::move(v));
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::efn4);
      CHECK(q.value() == 6);

      auto const s = std::move(std::as_const(v)) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  WHEN("optional const")
  {
    constexpr Xfn<fn::optional<int>> fn{};
    fn::optional<Xint> const v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(&Xint::ofn));

      auto const r = fn::invoke(and_then_t::apply{}, Xint::ofn, v);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::ofn);
      CHECK(q.value() == 2);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::not_invocable<lvalue>(&Xint::ofn1));

    WHEN("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(&Xint::ofn2));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::ofn2, v);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::ofn2);
      CHECK(q.value() == 4);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 4);
    }

    static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::not_invocable<rvalue>(&Xint::ofn3));

    WHEN("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::invocable<prvalue, crvalue, cvalue>(&Xint::ofn4));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::ofn4, std::move(v));
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::ofn4);
      CHECK(q.value() == 6);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  WHEN("optional mutable")
  {
    constexpr Xfn<fn::optional<int>> fn{};
    fn::optional<Xint> v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(Xint::ofn));

      auto const r = fn::invoke(and_then_t::apply{}, Xint::ofn, v);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::ofn);
      CHECK(q.value() == 2);
    }

    WHEN("lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable<lvalue>(&Xint::ofn1));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::ofn1, v);
      CHECK(r.value() == 3);

      auto const q = v | and_then(&Xint::ofn1);
      CHECK(q.value() == 3);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 3);
    }

    WHEN("const lvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable_with_any(&Xint::ofn2));

      // rvalue-ref
      auto const r = fn::invoke(and_then_t::apply{}, &Xint::ofn2, v);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::ofn2);
      CHECK(q.value() == 4);

      auto const s = std::as_const(v) | and_then(fn);
      CHECK(s.value() == 4);
    }

    WHEN("rvalue-ref")
    {
      static_assert(monadic_static_check<fn::and_then_t, decltype(v)>::invocable<prvalue, rvalue>(&Xint::ofn3));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::ofn3, std::move(v));
      CHECK(r.value() == 5);

      auto const q = std::move(v) | and_then(&Xint::ofn3);
      CHECK(q.value() == 5);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 5);
    }

    WHEN("const rvalue-ref")
    {
      static_assert(
          monadic_static_check<fn::and_then_t, decltype(v)>::invocable<prvalue, crvalue, cvalue>(&Xint::ofn4));

      auto const r = fn::invoke(and_then_t::apply{}, &Xint::ofn4, std::move(v));
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::ofn4);
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
  constexpr auto fnFail = [](int i) -> operand_t { return std::unexpected<Error>("Got " + std::to_string(i)); };
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

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place, 12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | and_then(fnFail)).error().what == "Got 12");
      }

      WHEN("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((a | and_then(fnXabs)).value().value == 4);
      }
    }

    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }

    WHEN("operand is pack")
    {
      using operand_t = fn::expected<fn::pack<int, double>, Error>;
      operand_t a{std::in_place, fn::pack{84, 0.5}};
      constexpr auto fnPack = [](int i, double d) constexpr -> fn::expected<int, Error> { return {i * d}; };
      using T = decltype(a | and_then(fnPack));
      static_assert(std::is_same_v<T, fn::expected<int, Error>>);

      WHEN("operand is value")
      {
        REQUIRE((a | and_then(fnPack)).value() == 42);
        WHEN("fail")
        {
          constexpr auto fnFail = [](int i, double d) constexpr -> fn::expected<int, Error> {
            return std::unexpected<Error>("Got " + std::to_string(i) + " and " + std::to_string(d));
          };
          using T = decltype(a | and_then(fnFail));
          static_assert(std::is_same_v<T, fn::expected<int, Error>>);
          REQUIRE((a | and_then(fnFail)).error().what == "Got 84 and 0.500000");
        }
      }

      WHEN("operand is error")
      {
        constexpr auto wrong = [](auto...) -> operand_t { throw 0; };
        REQUIRE((operand_t{std::unexpect, Error{"Not good"}} | and_then(wrong)).error().what == "Not good");
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place, 12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::in_place, 12} | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(operand_t{std::in_place, 12} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place, 12} | and_then(fnFail)).error().what == "Got 12");
      }

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place, 12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place, 12} | and_then(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
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
  auto fnFail = [&count]() -> operand_t { return std::unexpected<Error>("Got " + std::to_string(++count)); };
  auto fnXabs = [&count]() -> fn::expected<Xint, Error> { return {{++count}}; };

  static_assert(is::invocable_with_any(fnValue));
  static_assert(is::invocable_with_any([](auto...) -> operand_t { throw 0; }));        // allow generic call
  static_assert(is::invocable_with_any([]() -> operand_other_t { throw 0; }));         // allow conversion
  static_assert(is::not_invocable_with_any([]() -> operand_other_err_t { throw 0; })); // disallow error conversion
  static_assert(is::not_invocable_with_any([](int) -> operand_t { throw 0; }));        // bad arity
  static_assert(is::not_invocable_with_any([](int, int) -> operand_t { throw 0; }));   // bad arity

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{std::in_place};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (a | and_then(fnValue)).value();
      CHECK(count == 1);

      WHEN("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((a | and_then(fnFail)).error().what == "Got 2");
      }

      WHEN("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((a | and_then(fnXabs)).value().value == 2);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::unexpect, "Not good"};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a //
               | and_then(wrong))
                  .error()
                  .what
              == "Not good");
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{std::in_place} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      (operand_t{std::in_place} | and_then(fnValue)).value();
      CHECK(count == 1);

      WHEN("fail")
      {
        using T = decltype(operand_t{std::in_place} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE((operand_t{std::in_place} | and_then(fnFail)).error().what == "Got 2");
      }

      WHEN("change type")
      {
        using T = decltype(operand_t{std::in_place} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::expected<Xint, Error>>);
        REQUIRE((operand_t{std::in_place} | and_then(fnXabs)).value().value == 2);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::unexpect, "Not good"} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{std::unexpect, "Not good"} //
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

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((a | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(a | and_then(fnFail)).has_value());
      }

      WHEN("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::optional<Xint>>);
        REQUIRE((a | and_then(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      operand_t a{std::nullopt};
      using T = decltype(a | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(a | and_then(wrong)).has_value());
    }
  }

  WHEN("operand is pack")
  {
    using operand_t = fn::optional<fn::pack<int, double>>;
    operand_t a{std::in_place, fn::pack{84, 0.5}};
    constexpr auto fnPack = [](int i, double d) constexpr -> fn::optional<int> { return {i * d}; };
    using T = decltype(a | and_then(fnPack));
    static_assert(std::is_same_v<T, fn::optional<int>>);

    WHEN("operand is value")
    {
      REQUIRE((a | and_then(fnPack)).value() == 42);

      WHEN("fail")
      {
        constexpr auto fnFail = [](int, double) constexpr -> fn::optional<int> { return {std::nullopt}; };
        using T = decltype(a | and_then(fnFail));
        static_assert(std::is_same_v<T, fn::optional<int>>);
        REQUIRE(not(a | and_then(fnFail)).has_value());
      }
    }

    WHEN("operand is error")
    {
      constexpr auto wrong = [](auto...) -> operand_t { throw 0; };
      REQUIRE(not(operand_t{std::nullopt} | and_then(wrong)).has_value());
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE((operand_t{12} | and_then(fnValue)).value() == 13);

      WHEN("fail")
      {
        using T = decltype(operand_t{12} | and_then(fnFail));
        static_assert(std::is_same_v<T, operand_t>);
        REQUIRE(not(operand_t{12} | and_then(fnFail)).has_value());
      }

      WHEN("change type")
      {
        using T = decltype(operand_t{12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::optional<Xint>>);
        REQUIRE((operand_t{12} | and_then(fnXabs)).value().value == 4);
      }
    }
    WHEN("operand is error")
    {
      using T = decltype(operand_t{std::nullopt} | and_then(wrong));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(not(operand_t{std::nullopt} //
                  | and_then(wrong))
                     .has_value());
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

  WHEN("operand is lvalue")
  {
    WHEN("operand is value")
    {
      operand_t a{12};
      using T = decltype(a | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(*(a | and_then(fnValue)).get_ptr<int>() == 13);

      WHEN("change type")
      {
        using T = decltype(a | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::choice<Xint>>);
        REQUIRE((a | and_then(fnXabs)).get_ptr<Xint>()->value == 4);
      }
    }
  }

  WHEN("operand is rvalue")
  {
    WHEN("operand is value")
    {
      using T = decltype(operand_t{12} | and_then(fnValue));
      static_assert(std::is_same_v<T, operand_t>);
      REQUIRE(*(operand_t{12} | and_then(fnValue)).get_ptr<int>() == 13);

      WHEN("change type")
      {
        using T = decltype(operand_t{12} | and_then(fnXabs));
        static_assert(std::is_same_v<T, fn::choice<Xint>>);
        REQUIRE((operand_t{12} | and_then(fnXabs)).get_ptr<Xint>()->value == 4);
      }
    }
  }
}

TEST_CASE("constexpr and_then expected", "[and_then][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = fn::expected<int, Error>;

  WHEN("same value type")
  {
    constexpr auto fn = [](int i) constexpr noexcept -> T {
      if (i < 3)
        return {i + 1};
      return std::unexpected<Error>{Error::ThresholdExceeded};
    };
    constexpr auto r1 = T{0} | fn::and_then(fn);
    static_assert(r1.value() == 1);
    constexpr auto r2 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
    static_assert(r2.error() == Error::ThresholdExceeded);
  }

  WHEN("different value type")
  {
    using T1 = fn::expected<bool, Error>;
    constexpr auto fn = [](int i) constexpr noexcept -> T1 {
      if (i == 1)
        return {true};
      if (i == 0)
        return {false};
      return std::unexpected<Error>{Error::SomethingElse};
    };
    constexpr auto r1 = T{1} | fn::and_then(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<bool, Error> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::and_then(fn);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::and_then(fn);
    static_assert(r3.error() == Error::SomethingElse);
  }

  SUCCEED();
}

TEST_CASE("constexpr and_then expected with sum", "[and_then][constexpr][expected][sum]")
{
  enum class Error { ThresholdExceeded, SomethingElse, UnexpectedType };
  using T = fn::expected<fn::sum<Xint, int>, Error>;

  WHEN("same value type")
  {
    constexpr auto fn = fn::overload{[](int i) constexpr noexcept -> T {
                                       if (i < 3)
                                         return {i + 1};
                                       return std::unexpected<Error>{Error::ThresholdExceeded};
                                     },
                                     [](Xint v) constexpr noexcept -> T { return v; }};
    constexpr auto r1 = T{0} | fn::and_then(fn);
    static_assert(r1.value() == fn::sum{1});
    constexpr auto r2 = r1 | fn::and_then(fn) | fn::and_then(fn) | fn::and_then(fn);
    static_assert(r2.error() == Error::ThresholdExceeded);
  }

  WHEN("different value type")
  {
    using T1 = fn::expected<bool, Error>;
    constexpr auto fn
        = fn::overload{[](int i) constexpr noexcept -> T1 {
                         if (i == 1)
                           return {true};
                         if (i == 0)
                           return {false};
                         return std::unexpected<Error>{Error::SomethingElse};
                       },
                       [](Xint) constexpr noexcept -> T1 { return std::unexpected<Error>{Error::UnexpectedType}; }};
    constexpr auto r1 = T{1} | fn::and_then(fn);
    static_assert(std::is_same_v<decltype(r1), fn::expected<bool, Error> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::and_then(fn);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::and_then(fn);
    static_assert(r3.error() == Error::SomethingElse);
  }

  SUCCEED();
}

TEST_CASE("constexpr and_then graded monad", "[and_then][constexpr][expected][graded]")
{
  enum class Error { InvalidValue };
  using T = fn::expected<int, fn::sum<Error>>;

  WHEN("same error type")
  {
    constexpr auto fn1 = [](int i) -> fn::expected<int, int> {
      if (i < 2)
        return {i + 1};
      return std::unexpected<int>{i};
    };

    constexpr auto r1 = T{0} | fn::and_then(fn1);
    static_assert(std::is_same_v<decltype(r1), fn::expected<int, fn::sum<Error, int>> const>);
    static_assert(r1.value() == 1);
    constexpr auto r2 = r1 | fn::and_then(fn1);
    static_assert(r2.value() == 2);
    constexpr auto r3 = r2 | fn::and_then(fn1);
    static_assert(r3.error() == fn::sum{2});
    constexpr auto r4 = r3 | fn::and_then(fn1);
    static_assert(r4.error() == fn::sum{2});
  }

  WHEN("accummulate errors")
  {
    constexpr auto fn2 = [](int i) -> fn::expected<bool, Error> {
      if (i < 0 || i > 1)
        return std::unexpected<Error>{Error::InvalidValue};
      return {i == 1};
    };

    constexpr auto r2 = T{1} | fn::and_then(fn2);
    static_assert(std::is_same_v<decltype(r2), fn::expected<bool, fn::sum<Error>> const>);
    static_assert(r2.value());
    constexpr auto r3 = T{2} | fn::and_then(fn2);
    static_assert(r3.error() == fn::sum{Error::InvalidValue});

    constexpr auto fn3 = [](int i) -> fn::expected<int, int> { return {i + 1}; };
    constexpr auto r4 = r3 | fn::and_then(fn3);
    static_assert(std::is_same_v<decltype(r4), fn::expected<int, fn::sum<Error, int>> const>);
    static_assert(r4.error() == fn::sum{Error::InvalidValue});
    constexpr auto r5 = T{2} | fn::and_then(fn3);
    static_assert(r5.value() == 3);
  }

  SUCCEED();
}

TEST_CASE("constexpr and_then optional", "[and_then][constexpr][optional]")
{
  using T = fn::optional<int>;

  WHEN("same value type")
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
  }

  WHEN("different value type")
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
  }

  SUCCEED();
}

TEST_CASE("constexpr and_then optional with sum", "[and_then][constexpr][optional][sum]")
{
  using T = fn::optional<fn::sum<Xint, int>>;

  WHEN("same value type")
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
  }

  WHEN("different value type")
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
  }

  SUCCEED();
}

TEST_CASE("constexpr and_then choice", "[and_then][constexpr][choice]")
{
  using T = fn::choice<double, int>;

  WHEN("same value type")
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
  }

  WHEN("different value type")
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
  }

  SUCCEED();
}

namespace fn {
namespace {
struct Error {};
struct Xerror final : Error {};
struct Value final {};

template <typename T> constexpr auto fn_int = [](int) -> T { throw 0; };
template <typename T> constexpr auto fn_generic = [](auto &&...) -> T { throw 0; };
template <typename T> constexpr auto fn_int_lvalue = [](int &) -> T { throw 0; };
template <typename T> constexpr auto fn_int_const_lvalue = [](int const &) -> T { throw 0; };
template <typename T> constexpr auto fn_int_rvalue = [](int &&) -> T { throw 0; };
template <typename T> constexpr auto fn_int_const_rvalue = [](int const &&) -> T { throw 0; };

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
// clang-format on
} // namespace fn
