// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "static_check.hpp"

#include "functional/and_then.hpp"
#include "functional/functor.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <utility>

using namespace util;

namespace {
struct Error final {
  std::string what;
};

struct Xint final {
  int value;

  static auto efn(Xint const &self) noexcept -> std::expected<int, Error> { return {self.value}; }
  auto efn1() & noexcept -> std::expected<int, Error> { return {value + 1}; }
  auto efn2() const & noexcept -> std::expected<int, Error> { return {value + 2}; }
  auto efn3() && noexcept -> std::expected<int, Error> { return {value + 3}; }
  auto efn4() const && noexcept -> std::expected<int, Error> { return {value + 4}; }

  static auto ofn(Xint const &self) noexcept -> std::optional<int> { return {self.value}; }
  auto ofn1() & noexcept -> std::optional<int> { return {value + 1}; }
  auto ofn2() const & noexcept -> std::optional<int> { return {value + 2}; }
  auto ofn3() && noexcept -> std::optional<int> { return {value + 3}; }
  auto ofn4() const && noexcept -> std::optional<int> { return {value + 4}; }
};

template <typename R> struct Xfn final {
  auto operator()(Xint &v) const noexcept -> R { return {v.value + 1}; }
  auto operator()(Xint const &v) const noexcept -> R { return {v.value + 2}; }
  auto operator()(Xint &&v) const noexcept -> R { return {v.value + 3}; }
  auto operator()(Xint const &&v) const noexcept -> R { return {v.value + 4}; }
};

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error>, decltype(Xint::efn)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const, decltype(Xint::efn)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &, decltype(Xint::efn)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &, decltype(Xint::efn)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &&, decltype(Xint::efn)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &&, decltype(Xint::efn)>);

static_assert(not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error>,
                                        decltype(&Xint::efn1)>); // cannot bind rvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const,
                              decltype(&Xint::efn1)>); // cannot bind const
                                                       // rvalue
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &, decltype(&Xint::efn1)>);
static_assert( //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &,
                              decltype(&Xint::efn1)>); // cannot bind const
static_assert(not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &&,
                                        decltype(&Xint::efn1)>); // cannot bind rvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &&,
                              decltype(&Xint::efn1)>); // cannot bind const rvalue
static_assert(                                         //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &,
                              decltype(&Xint::efn1)>); // cannot bind as non-const

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error>, decltype(&Xint::efn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const, decltype(&Xint::efn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &, decltype(&Xint::efn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &, decltype(&Xint::efn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &&, decltype(&Xint::efn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &&, decltype(&Xint::efn2)>);

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error>, decltype(&Xint::efn3)>);
static_assert(not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const,
                                        decltype(&Xint::efn3)>); // cannot bind const
static_assert(not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &,
                                        decltype(&Xint::efn3)>); // cannot bind lvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &,
                              decltype(&Xint::efn3)>); // cannot bind const lvalue
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &&, decltype(&Xint::efn3)>);
static_assert( //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &&,
                              decltype(&Xint::efn3)>); // cannot bind const

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error>, decltype(&Xint::efn4)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const, decltype(&Xint::efn4)>);
static_assert(not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &,
                                        decltype(&Xint::efn4)>); // cannot bind lvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &,
                              decltype(&Xint::efn4)>); // cannot bind const lvalue
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> &&, decltype(&Xint::efn4)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::expected<Xint, Error> const &&, decltype(&Xint::efn4)>);

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint>, decltype(Xint::ofn)>);
static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const, decltype(Xint::ofn)>);
static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &, decltype(Xint::ofn)>);
static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &, decltype(Xint::ofn)>);
static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &&, decltype(Xint::ofn)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &&, decltype(Xint::ofn)>);

static_assert(not fn::monadic_invocable<fn::and_then_t, std::optional<Xint>,
                                        decltype(&Xint::ofn1)>); // cannot bind rvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const,
                              decltype(&Xint::ofn1)>); // cannot bind const
                                                       // rvalue
static_assert(                                         //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &, decltype(&Xint::ofn1)>);
static_assert( //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &,
                              decltype(&Xint::ofn1)>); // cannot bind const
static_assert(not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &&,
                                        decltype(&Xint::ofn1)>); // cannot bind rvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &&,
                              decltype(&Xint::ofn1)>); // cannot bind const
                                                       // rvalue
static_assert(                                         //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &,
                              decltype(&Xint::ofn1)>); // cannot bind as
                                                       // non-const

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint>, decltype(&Xint::ofn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const, decltype(&Xint::ofn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &, decltype(&Xint::ofn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &, decltype(&Xint::ofn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &&, decltype(&Xint::ofn2)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &&, decltype(&Xint::ofn2)>);

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint>, decltype(&Xint::ofn3)>);
static_assert(not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const,
                                        decltype(&Xint::ofn3)>); // cannot bind const
static_assert(not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &,
                                        decltype(&Xint::ofn3)>); // cannot bind lvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &,
                              decltype(&Xint::ofn3)>); // cannot bind const
                                                       // lvalue
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &&, decltype(&Xint::ofn3)>);
static_assert( //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &&,
                              decltype(&Xint::ofn3)>); // cannot bind const

static_assert( //
    fn::monadic_invocable<fn::and_then_t, std::optional<Xint>, decltype(&Xint::ofn4)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const, decltype(&Xint::ofn4)>);
static_assert(not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &,
                                        decltype(&Xint::ofn4)>); // cannot bind lvalue
static_assert(                                                   //
    not fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &,
                              decltype(&Xint::ofn4)>); // cannot bind const
                                                       // lvalue
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> &&, decltype(&Xint::ofn4)>);
static_assert(fn::monadic_invocable<fn::and_then_t, std::optional<Xint> const &&, decltype(&Xint::ofn4)>);
} // namespace

TEST_CASE("and_then_member", "[and_then][member_functions]")
{
  using namespace fn;

  WHEN("expected const")
  {
    constexpr Xfn<std::expected<int, Error>> fn{};
    std::expected<Xint, Error> const v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(fn::monadic_invocable<and_then_t, decltype(v), decltype(Xint::efn)>);

      auto const r = std::invoke(and_then_t::apply{}, v, Xint::efn);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::efn);
      CHECK(q.value() == 2);
    }

    static_assert( //
        not fn::monadic_invocable<fn::and_then_t, decltype(v) &, decltype(&Xint::efn1)>);

    WHEN("const lvalue-ref")
    {
      static_assert( //
          fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::efn2)>);

      auto const r = std::invoke(and_then_t::apply{}, v, &Xint::efn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::efn2);
      CHECK(q.value() == 4);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 4);
    }

    static_assert( //
        not fn::monadic_invocable<fn::and_then_t, decltype(v) &&, decltype(&Xint::efn3)>);

    WHEN("const rvalue-ref")
    {
      static_assert( //
          fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::efn4)>);

      auto const r = std::invoke(and_then_t::apply{}, std::move(v), &Xint::efn4);
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::efn4);
      CHECK(q.value() == 6);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  WHEN("expected mutable")
  {
    constexpr Xfn<std::expected<int, Error>> fn{};
    std::expected<Xint, Error> v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(Xint::efn)>);

      auto const r = std::invoke(and_then_t::apply{}, v, Xint::efn);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::efn);
      CHECK(q.value() == 2);
    }

    WHEN("lvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v) &, decltype(&Xint::efn1)>);
      auto const r = std::invoke(and_then_t::apply{}, v, &Xint::efn1);
      CHECK(r.value() == 3);

      auto const q = v | and_then(&Xint::efn1);
      CHECK(q.value() == 3);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 3);
    }

    WHEN("const lvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::efn2)>);
      // rvalue-ref
      auto const r = std::invoke(and_then_t::apply{}, v, &Xint::efn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::efn2);
      CHECK(q.value() == 4);

      auto const s = std::as_const(v) | and_then(fn);
      CHECK(s.value() == 4);
    }

    WHEN("rvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::efn3)>);

      auto const r = std::invoke(and_then_t::apply{}, std::move(v), &Xint::efn3);
      CHECK(r.value() == 5);

      auto const q = std::move(v) | and_then(&Xint::efn3);
      CHECK(q.value() == 5);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 5);
    }

    WHEN("const rvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::efn4)>);

      auto const r = std::invoke(and_then_t::apply{}, std::move(v), &Xint::efn4);
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::efn4);
      CHECK(q.value() == 6);

      auto const s = std::move(std::as_const(v)) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  WHEN("optional const")
  {
    constexpr Xfn<std::optional<int>> fn{};
    std::optional<Xint> const v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(fn::monadic_invocable<and_then_t, decltype(v), decltype(Xint::ofn)>);

      auto const r = std::invoke(and_then_t::apply{}, v, Xint::ofn);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::ofn);
      CHECK(q.value() == 2);
    }

    static_assert( //
        not fn::monadic_invocable<fn::and_then_t, decltype(v) &, decltype(&Xint::ofn1)>);

    WHEN("const lvalue-ref")
    {
      static_assert( //
          fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::ofn2)>);

      auto const r = std::invoke(and_then_t::apply{}, v, &Xint::ofn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::ofn2);
      CHECK(q.value() == 4);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 4);
    }

    static_assert( //
        not fn::monadic_invocable<fn::and_then_t, decltype(v) &&, decltype(&Xint::ofn3)>);

    WHEN("const rvalue-ref")
    {
      static_assert( //
          fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::ofn4)>);

      auto const r = std::invoke(and_then_t::apply{}, std::move(v), &Xint::ofn4);
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::ofn4);
      CHECK(q.value() == 6);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }

  WHEN("optional mutable")
  {
    constexpr Xfn<std::optional<int>> fn{};
    std::optional<Xint> v{std::in_place, Xint{2}};

    WHEN("static fn")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(Xint::ofn)>);

      auto const r = std::invoke(and_then_t::apply{}, v, Xint::ofn);
      CHECK(r.value() == 2);

      auto const q = v | and_then(&Xint::ofn);
      CHECK(q.value() == 2);
    }

    WHEN("lvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v) &, decltype(&Xint::ofn1)>);
      auto const r = std::invoke(and_then_t::apply{}, v, &Xint::ofn1);
      CHECK(r.value() == 3);

      auto const q = v | and_then(&Xint::ofn1);
      CHECK(q.value() == 3);

      auto const s = v | and_then(fn);
      CHECK(s.value() == 3);
    }

    WHEN("const lvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::ofn2)>);
      // rvalue-ref
      auto const r = std::invoke(and_then_t::apply{}, v, &Xint::ofn2);
      CHECK(r.value() == 4);

      auto const q = v | and_then(&Xint::ofn2);
      CHECK(q.value() == 4);

      auto const s = std::as_const(v) | and_then(fn);
      CHECK(s.value() == 4);
    }

    WHEN("rvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::ofn3)>);

      auto const r = std::invoke(and_then_t::apply{}, std::move(v), &Xint::ofn3);
      CHECK(r.value() == 5);

      auto const q = std::move(v) | and_then(&Xint::ofn3);
      CHECK(q.value() == 5);

      auto const s = std::move(v) | and_then(fn);
      CHECK(s.value() == 5);
    }

    WHEN("const rvalue-ref")
    {
      static_assert(fn::monadic_invocable<fn::and_then_t, decltype(v), decltype(&Xint::ofn4)>);

      auto const r = std::invoke(and_then_t::apply{}, std::move(v), &Xint::ofn4);
      CHECK(r.value() == 6);

      auto const q = std::move(v) | and_then(&Xint::ofn4);
      CHECK(q.value() == 6);

      auto const s = std::move(std::as_const(v)) | and_then(fn);
      CHECK(s.value() == 6);
    }
  }
}

TEST_CASE("and_then", "[and_then][expected][expected_value]")
{
  using namespace fn;

  using operand_t = std::expected<int, Error>;
  constexpr auto fnValue = [](int i) -> operand_t { return {i + 1}; };

  using is = static_check::bind_right<and_then_t>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<and_then_t, operand_t, decltype(fnValue)>);
  static_assert(is::invocable<operand_t>([](auto...) -> operand_t { throw 0; }));           // allow generic call
  static_assert(is::invocable<operand_t>([](unsigned) -> operand_t { throw 0; }));          // allow conversion
  static_assert(not is::invocable<operand_t const>([](int &&) -> operand_t { throw 0; }));  // disallow removing const
  static_assert(not is::invocable<operand_t &>([](int &&) -> operand_t { throw 0; }));      // disallow move from lvalue
  static_assert(is::invocable<operand_t &>([](int &) -> operand_t { throw 0; }));           // allow lvalue binding
  static_assert(not is::invocable<operand_t const &>([](int &) -> operand_t { throw 0; })); // disallow removing const

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<and_then_t, operand_t &&, decltype(fnValue)>);
  static_assert(is::invocable<operand_t &&>([](int &&) -> operand_t { throw 0; })); // alow move from rvalue
  static_assert(
      not is::invocable<operand_t &&>([](int &) -> operand_t { throw 0; })); // disallow lvalue binding to rvalue
  static_assert(is::invocable<operand_t &&>(
      [](int const &) -> operand_t { throw 0; }));                 // allow const lvalue ref binding to rvalue
  static_assert(not is::invocable<operand_t>([](std::string) {})); // wrong type
  static_assert(not is::invocable<operand_t>([]() {}));            // wrong arity
  static_assert(not is::invocable<operand_t>([](int, int) {}));    // wrong arity

  constexpr auto wrong = [](int) -> operand_t { throw 0; };
  constexpr auto fnFail = [](int i) -> operand_t { return std::unexpected<Error>("Got " + std::to_string(i)); };
  constexpr auto fnXabs = [](int i) -> std::expected<Xint, Error> { return {{std::abs(8 - i)}}; };

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
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
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
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
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

  using operand_t = std::expected<void, Error>;
  int count = 0;
  auto fnValue = [&count]() -> operand_t {
    count += 1;
    return {};
  };

  using is = static_check::bind_right<and_then_t>;

  static_assert(monadic_invocable<and_then_t, operand_t, decltype(fnValue)>);
  static_assert(is::invocable<operand_t>([](auto...) -> operand_t { return {}; }));        // allow generic call
  static_assert(is::invocable<operand_t>([]() -> operand_t { return {}; }));               // allow conversion
  static_assert(not is::invocable<operand_t>([](auto) -> operand_t { return {}; }));       // wrong arity
  static_assert(not is::invocable<operand_t>([](auto, auto) -> operand_t { return {}; })); // wrong arity

  constexpr auto wrong = []() -> operand_t { throw 0; };
  auto fnFail = [&count]() -> operand_t { return std::unexpected<Error>("Got " + std::to_string(++count)); };
  auto fnXabs = [&count]() -> std::expected<Xint, Error> { return {{++count}}; };

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
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
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
        static_assert(std::is_same_v<T, std::expected<Xint, Error>>);
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

TEST_CASE("and_then", "[and_then][optional]")
{
  using namespace fn;

  using operand_t = std::optional<int>;
  constexpr auto fnValue = [](int i) -> operand_t { return {i + 1}; };

  using is = static_check::bind_right<and_then_t>;

  // lvalue operand
  // --------------
  static_assert(monadic_invocable<and_then_t, operand_t, decltype(fnValue)>);
  static_assert(is::invocable<operand_t>([](auto...) -> operand_t { throw 0; }));           // allow generic call
  static_assert(is::invocable<operand_t>([](unsigned) -> operand_t { throw 0; }));          // allow conversion
  static_assert(not is::invocable<operand_t const>([](int &&) -> operand_t { throw 0; }));  // disallow removing const
  static_assert(not is::invocable<operand_t &>([](int &&) -> operand_t { throw 0; }));      // disallow move from lvalue
  static_assert(is::invocable<operand_t &>([](int &) -> operand_t { throw 0; }));           // allow lvalue binding
  static_assert(not is::invocable<operand_t const &>([](int &) -> operand_t { throw 0; })); // disallow removing const
  static_assert(not is::invocable<operand_t>([](std::string) {}));                          // wrong type
  static_assert(not is::invocable<operand_t>([]() {}));                                     // wrong arity
  static_assert(not is::invocable<operand_t>([](int, int) {}));                             // wrong arity

  // rvalue operand
  // --------------
  static_assert(monadic_invocable<and_then_t, operand_t &&, decltype(fnValue)>);
  static_assert(is::invocable<operand_t &&>([](int &&) -> operand_t { throw 0; })); // alow move from rvalue
  static_assert(
      not is::invocable<operand_t &&>([](int &) -> operand_t { throw 0; })); // disallow lvalue binding to rvalue
  static_assert(is::invocable<operand_t &&>(
      [](int const &) -> operand_t { throw 0; })); // allow const lvalue ref binding to rvalue

  constexpr auto wrong = [](int) -> operand_t { throw 0; };
  constexpr auto fnFail = [](int) -> operand_t { return {}; };
  constexpr auto fnXabs = [](int i) -> std::optional<Xint> { return {{std::abs(8 - i)}}; };

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
        static_assert(std::is_same_v<T, std::optional<Xint>>);
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
        static_assert(std::is_same_v<T, std::optional<Xint>>);
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

TEST_CASE("constexpr and_then expected", "[and_then][constexpr][expected]")
{
  enum class Error { ThresholdExceeded, SomethingElse };
  using T = std::expected<int, Error>;

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
    using T1 = std::expected<bool, Error>;
    constexpr auto fn = [](int i) constexpr noexcept -> T1 {
      if (i == 1)
        return {true};
      if (i == 0)
        return {false};
      return std::unexpected<Error>{Error::SomethingElse};
    };
    constexpr auto r1 = T{1} | fn::and_then(fn);
    static_assert(std::is_same_v<decltype(r1), std::expected<bool, Error> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::and_then(fn);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::and_then(fn);
    static_assert(r3.error() == Error::SomethingElse);
  }

  SUCCEED();
}

TEST_CASE("constexpr and_then optional", "[and_then][constexpr][optional]")
{
  using T = std::optional<int>;

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
    using T1 = std::optional<bool>;
    constexpr auto fn = [](int i) constexpr noexcept -> T1 {
      if (i == 1)
        return {true};
      if (i == 0)
        return {false};
      return {};
    };
    constexpr auto r1 = T{1} | fn::and_then(fn);
    static_assert(std::is_same_v<decltype(r1), std::optional<bool> const>);
    static_assert(r1.value() == true);
    constexpr auto r2 = T{0} | fn::and_then(fn);
    static_assert(r2.value() == false);
    constexpr auto r3 = T{2} | fn::and_then(fn);
    static_assert(not r3.has_value());
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

static_assert(invocable_and_then<decltype(fn_int<std::expected<Value, Error>>), std::expected<int, Error>>);
static_assert(invocable_and_then<decltype(fn_int<std::expected<void, Error>>), std::expected<int, Error>>);
static_assert(not invocable_and_then<decltype(fn_int<std::expected<int, Xerror>>),
                                     std::expected<int, Error>>); // different error_type
static_assert(not invocable_and_then<decltype(fn_int<std::expected<int, Error>>),
                                     std::expected<Value, Error>>); // wrong parameter type
static_assert(invocable_and_then<decltype(fn_generic<std::expected<int, Error>>), std::expected<Value, Error>>);
static_assert(not invocable_and_then<decltype(fn_generic<std::expected<int, Xerror>>),
                                     std::expected<Value, Error>>); // different error_type
static_assert(invocable_and_then<decltype(fn_generic<std::expected<int, Error>>), std::expected<void, Error>>);
static_assert(invocable_and_then<decltype(fn_generic<std::expected<void, Error>>), std::expected<void, Error>>);
static_assert(invocable_and_then<decltype(fn_generic<std::expected<Value, Error>>), std::expected<void, Error>>);
static_assert(not invocable_and_then<decltype(fn_generic<std::expected<int, Xerror>>),
                                     std::expected<void, Error>>); // different error_type
static_assert(not invocable_and_then<decltype(fn_generic<std::expected<void, Xerror>>),
                                     std::expected<void, Error>>); // different error_type
static_assert(not invocable_and_then<decltype(fn_generic<std::expected<int, Error>>),
                                     std::optional<Value>>); // mixed optional and expected
static_assert(not invocable_and_then<decltype(fn_generic<std::expected<int, Xerror>>),
                                     std::optional<int>>); // mixed optional and expected
static_assert(not invocable_and_then<decltype(fn_generic<std::optional<int>>),
                                     std::expected<Value, Error>>); // mixed optional and expected
static_assert(invocable_and_then<decltype(fn_generic<std::optional<int>>), std::optional<Value>>);
static_assert(invocable_and_then<decltype(fn_generic<std::optional<Value>>), std::optional<int>>);
static_assert(not invocable_and_then<decltype(fn_int_lvalue<std::expected<Value, Error>>),
                                     std::expected<int, Error>>); // cannot bind temporary to lvalue
static_assert(invocable_and_then<decltype(fn_int_lvalue<std::expected<Value, Error>>), std::expected<int, Error> &>);
static_assert(invocable_and_then<decltype(fn_int_rvalue<std::expected<Value, Error>>), std::expected<int, Error>>);
static_assert(not invocable_and_then<decltype(fn_int_rvalue<std::expected<Value, Error>>),
                                     std::expected<int, Error> &>); // cannot bind lvalue to rvalue-ref
} // namespace fn
