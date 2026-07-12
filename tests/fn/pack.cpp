// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "util/helper_types.hpp"

#include <fn/functional.hpp>
#include <fn/optional.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>

#include <catch2/catch_all.hpp>

#include <tuple>
#include <utility>

namespace {
struct A final {
  int v = 0;
};

template <typename V, typename Fn>
concept pack_check = requires(V v, Fn fn) { FWD(v).invoke(FWD(fn), A{}); };

template <fn::pack<int, double> P> struct pack_nttp final {};
template <fn::some_pack auto P> struct some_pack_nttp final {};
template <fn::some_pack auto P> auto read_nttp() { return fn::get<0>(P); }

template <typename V, typename R, typename Fn>
concept can_invoke_r = requires(V v, Fn fn) { FWD(v).template invoke_r<R>(FWD(fn)); };

template <typename V, typename T, typename... Args>
concept can_append_in_place = requires(V v, Args... args) { FWD(v).append(std::in_place_type<T>, args...); };

// A pack element whose copy and move can both throw, while the pack member relocating it promises
// noexcept regardless. Witnesses the #280 tripwires below.
using Throwing = helper_t<prop::throw_copy | prop::throw_move>;

} // namespace

TEST_CASE("pack", "[pack]")
{
  using fn::pack;

  using T = pack<int, int const, int &, int const &>;
  int val1 = 15;
  int const val2 = 92;
  T v{3, 14, val1, val2};
  CHECK(v.size == 4);
  static_assert(pack_check<T, decltype(([](auto &&...) {}))>);            // generic call
  static_assert(pack_check<T, decltype(([](int, int, int, int, A) {}))>); // pass everything by value
  static_assert(pack_check<T, decltype(([](int const &, int const &, int const &, int const &, A const &) {
                           }))>); // pass everything by const reference
  static_assert(pack_check<T, decltype(([](int, int, int &, int const &, A) {}))>); // bind lvalues
  static_assert(
      pack_check<T, decltype(([](int &&, int const &&, int &, int const &, A &&) {}))>); // bind rvalues and lvalues
  static_assert(pack_check<T, decltype(([](int const, int const, int const &, int const &, A const) {
                           }))>); // pass values or lvalues promoted to     const

  static_assert(not pack_check<T, decltype(([](int &, auto &&...) {}))>); // cannot bind rvalue to lvalue reference
  static_assert(
      not pack_check<T, decltype(([](int, int, int &&, int, A) {}))>); // cannot bind lvalue to rvalue reference
  static_assert(
      not pack_check<T, decltype(([](int, int, int, int &&, A) {}))>); // cannot bind const lvalue to rvalue reference
  static_assert(not pack_check<T, decltype(([](int, int, int, int const &&, A) {}))>); // cannot bind const lvalue to
                                                                                       // const rvalue      reference
  static_assert(not pack_check<T, decltype(([](int, int &&, int, int, A &&) {}))>);    // cannot bind const rvalue to
                                                                                       // non-const rvalue  reference
  static_assert(not pack_check<T, decltype(([](int, int, int, int, int) {}))>);        // bad type
  static_assert(not pack_check<T, decltype(([](auto, auto, auto, auto) {}))>);         // bad arity
  static_assert(not pack_check<T, decltype(([](auto, auto, auto, auto, auto, auto) {}))>); // bad arity

  constexpr auto fn = [](auto... args) noexcept -> int { return (0 + ... + args); };
  CHECK(v.invoke(fn) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke(fn, FWD(v)) == 3 + 14 + 15 + 92);
  CHECK(v.invoke(fn, 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::invoke(fn, FWD(v), 65, 35) == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(fn::invoke(fn, fn::pack{3, 14}, 15, 92) == 3 + 14 + 15 + 92);

  constexpr auto fn0 = [](int i, int j, int k, int l, A) noexcept -> int { return (i + j + k + l); };
  CHECK(v.invoke(fn0, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke(fn0, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::invoke(fn0, fn::pack{3, 14, 15, 92}, A{}) == 3 + 14 + 15 + 92);

  A a;
  constexpr auto fn1 = [](int i, int j, int k, int l, A &dest) noexcept -> A & {
    dest.v = (i + j + k + l);
    return dest;
  };
  CHECK(v.invoke(fn1, a).v == 3 + 14 + 15 + 92);
  CHECK(v.invoke_r<A>(fn1, a).v == 3 + 14 + 15 + 92);
  CHECK(v.invoke_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  CHECK(fn::invoke_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, FWD(v), 65, 35)
        == 3 + 14 + 15 + 92 + 65 + 35);
  static_assert(
      fn::invoke_r<long>([](auto... args) noexcept -> int { return (0 + ... + args); }, fn::pack{3, 14}, 15, 92)
      == 3 + 14 + 15 + 92);
  CHECK(v.invoke_r<long>(fn0, A{}) == 3 + 14 + 15 + 92);
  CHECK(fn::invoke_r<long>(fn0, FWD(v), A{}) == 3 + 14 + 15 + 92);
  static_assert(fn::invoke_r<long>(fn0, fn::pack{3, 14, 15, 92}, A{}) == 3 + 14 + 15 + 92);

  static_assert(std::is_same_v<decltype(v.invoke(fn1, a)), A &>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn1, a)), A>);

  constexpr auto fn2 = [](int, int, int, int, A &&dest) noexcept -> A && { return std::move(dest); };
  static_assert(std::is_same_v<decltype(v.invoke(fn2, std::move(a))), A &&>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn2, std::move(a))), A>);

  constexpr auto fn3 = [](int, int, int, int, A &&dest) noexcept -> A { return dest; };
  static_assert(std::is_same_v<decltype(v.invoke(fn3, std::move(a))), A>);
  static_assert(std::is_same_v<decltype(v.invoke_r<A>(fn3, std::move(a))), A>);

  static_assert(pack<>::size == 0);

  static_assert(std::same_as<decltype(fn::pack{}), fn::pack<>>);
  static_assert(std::same_as<decltype(fn::pack{12}), fn::pack<int>>);
  static_assert(std::same_as<decltype(fn::pack{a}), fn::pack<A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, a}), fn::pack<int, A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::as_const(a)}), fn::pack<int, A const &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::move(a)}), fn::pack<int, A>>);

  constexpr auto c1 = fn::as_pack();
  static_assert(std::same_as<decltype(c1), fn::pack<> const>);
  constexpr auto c2 = fn::as_pack(true, 12);
  static_assert(std::same_as<decltype(c2), fn::pack<bool, int> const>);
  static_assert(c2.invoke([](auto i, auto j) {
    if constexpr (std::is_same_v<decltype(i), bool> && std::is_same_v<decltype(j), int>)
      return i && j == 12;
    else
      return false;
  }));

  // pack is a structural type: a constexpr pack can be a template parameter, with
  // template-argument equivalence comparing element-wise
  constexpr pack<int, double> s1{3, 0.5};
  constexpr pack<int, double> s2{3, 0.5};
  constexpr pack<int, double> s3{4, 0.5};
  static_assert(std::is_same_v<pack_nttp<s1>, pack_nttp<s2>>);
  static_assert(not std::is_same_v<pack_nttp<s1>, pack_nttp<s3>>);
  static_assert(std::is_same_v<some_pack_nttp<s1>, some_pack_nttp<s2>>);
  static_assert(not std::is_same_v<some_pack_nttp<s1>, some_pack_nttp<s3>>);
  CHECK(read_nttp<s1>() == 3); // the template-parameter object is usable at runtime

  SECTION("invoke_r return conversion")
  {
    struct NotFromInt final {
      explicit NotFromInt(std::nullptr_t) {}
    };
    constexpr auto sum_fn = [](auto... args) noexcept -> int { return (0 + ... + args); };

    static_assert(can_invoke_r<T &, long, decltype(sum_fn)>);
    static_assert(not can_invoke_r<T &, NotFromInt, decltype(sum_fn)>); // int does not convert to it
    SUCCEED();
  }

  SECTION("noexcept")
  {
    // GAP #280: invoke and invoke_r are unconditionally noexcept whatever the callback promises,
    // so a throwing callback terminates instead of propagating.
    constexpr auto throwing = [](auto &&...) noexcept(false) -> int { return 0; };
    static_assert(not noexcept(throwing()));
    static_assert(noexcept(v.invoke(throwing)));
    static_assert(noexcept(std::as_const(v).invoke(throwing)));
    static_assert(noexcept(std::move(v).invoke(throwing)));
    static_assert(noexcept(std::move(std::as_const(v)).invoke(throwing)));
    static_assert(noexcept(v.invoke_r<long>(throwing)));
    static_assert(noexcept(std::move(v).invoke_r<long>(throwing)));

    // GAP #280 (converse): the as_pack lifts carry no noexcept specifier, so they report
    // noexcept(false) even where nothing can throw.
    static_assert(not noexcept(fn::as_pack()));
    static_assert(not noexcept(fn::as_pack(true, 12)));
    SUCCEED();
  }

  SECTION("constexpr")
  {
    constexpr fn::pack<int, int> v2{3, 14};
    constexpr auto r2 = v2.invoke([](auto &&...args) constexpr noexcept -> int { return (0 + ... + args); });
    static_assert(r2 == 3 + 14);
    SUCCEED();
  }
}

TEST_CASE("append value categories", "[pack][append]")
{
  using fn::pack;

  struct B {
    constexpr explicit B(int i) : v(i) {}
    constexpr B(int i, int j) : v(i * j) {}

    int v = 0;
  };

  struct C final : B {
    constexpr C() : B(30) {}
  };

  using T = pack<int, std::string_view, A>;
  T s{12, "bar", 42};

  static_assert(T::size == 3);
  constexpr auto check = [](int i, std::string_view s, A a, B const &b) noexcept -> bool {
    return i == 12 && s == std::string("bar") && a.v == 42 && b.v == 30;
  };

  SECTION("explicit type selection")
  {
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B>, 5, 6)), T::append_type<B>>);
    static_assert(std::same_as<T::append_type<B>, pack<int, std::string_view, A, B>>);
    static_assert(decltype(s.append(std::in_place_type<B>, 5, 6))::size == 4);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B const &>, c1)), T::append_type<B const &>>);
    static_assert(std::same_as<T::append_type<B const &>, pack<int, std::string_view, A, B const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B &>, c2)), T::append_type<B &>>);
    static_assert(std::same_as<T::append_type<B &>, pack<int, std::string_view, A, B &>>);

    SECTION("constructor takes parameters")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::move(s).append(std::in_place_type<B>, 5, 6).invoke(check));
    }

    SECTION("constructor takes parameters invoke_r")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
      CHECK(std::move(s).append(std::in_place_type<B>, 5, 6).invoke_r<bool>(check));
    }

    SECTION("default constructor")
    {
      CHECK(s.append(std::in_place_type<C>).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<C>).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).invoke(check));
      CHECK(std::move(s).append(std::in_place_type<C>).invoke(check));
    }

    SECTION("default constructor invoke_r")
    {
      CHECK(s.append(std::in_place_type<C>).invoke_r<int>(check) == 1);
      CHECK(std::as_const(s).append(std::in_place_type<C>).invoke_r<int>(check) == 1);
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).invoke_r<int>(check) == 1);
      CHECK(std::move(s).append(std::in_place_type<C>).invoke_r<int>(check) == 1);
    }
  }

  SECTION("deduced type")
  {
    static_assert(std::same_as<decltype(s.append(B{5, 6})), T::append_type<B>>);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(c1)), T::append_type<C const &>>);
    static_assert(std::same_as<T::append_type<C const &>, pack<int, std::string_view, A, C const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(c2)), T::append_type<C &>>);
    static_assert(std::same_as<T::append_type<C &>, pack<int, std::string_view, A, C &>>);

    CHECK(s.append(B{30}).invoke(check));
    CHECK(std::as_const(s).append(B{30}).invoke(check));
    CHECK(std::move(std::as_const(s)).append(B{30}).invoke(check));
    CHECK(std::move(s).append(B{30}).invoke(check));
  }

  SECTION("pack on the right side, deduced")
  {
    constexpr fn::pack<bool, int, B> a{true, 3, B{14}};
    constexpr fn::pack<C, B> b{C{}, B{3, 4}};
    constexpr auto c1 = a.append(b);
    static_assert(std::same_as<decltype(c1), fn::pack<bool, int, B, C, B> const>);
    static_assert(c1.invoke([](bool i, int j, B const &b1, C const &c, B const &b2) {
      return i && j == 3 && b1.v == 14 && c.v == 30 && b2.v == 12;
    }));

    auto c2 = a.append(fn::pack{C{}, B{4, 5}});
    static_assert(std::same_as<decltype(c2), fn::pack<bool, int, B, C, B>>);
    CHECK(c2.invoke([](bool i, int j, B const &b1, C const &c, B const &b2) {
      return i && j == 3 && b1.v == 14 && c.v == 30 && b2.v == 20;
    }));
  }

  SECTION("constraints")
  {
    static_assert(can_append_in_place<T &, B, int>);
    static_assert(can_append_in_place<T &, B, int, int>);
    static_assert(not can_append_in_place<T &, B, char const *>); // B is not constructible from it

    // GAP #283: given no arguments and an element with no default constructor, the in_place overload
    // drops out on its is_constructible_v conjunct and the deduced-Arg overload picks the call up
    // instead - silently appending the TAG as an element rather than failing. So the call below is
    // "viable" for entirely the wrong reason, and the element type says so.
    static_assert(can_append_in_place<T &, B>);
    static_assert(std::same_as<decltype(std::declval<T &>().append(std::in_place_type<B>)),
                               T::append_type<std::in_place_type_t<B> const &>>);
    // A default-constructible element takes the intended path - see SECTION("default constructor").
    static_assert(std::same_as<decltype(std::declval<T &>().append(std::in_place_type<C>)), T::append_type<C>>);

    // GAP #282: a pack never holds a sum, but merely ASKING whether one can be appended is a hard
    // error on gcc (ambiguous partial specialization of _pack_append), so no negative probe is
    // portable here until that is fixed.

    SUCCEED();
  }

  SECTION("noexcept")
  {
    // GAP #280: every append overload is unconditionally noexcept, though _append both constructs
    // the new element and relocates every existing one into the new pack.
    using P = pack<Throwing>;
    static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);
    static_assert(not std::is_nothrow_move_constructible_v<Throwing>);

    // constructing the appended element by a throwing copy
    static_assert(
        noexcept(std::declval<pack<int> &>().append(std::in_place_type<Throwing>, std::declval<Throwing const &>())));
    // ... and relocating an existing throwing element, even where the appended one cannot throw
    static_assert(noexcept(std::declval<P &>().append(std::in_place_type<int>, 1)));
    static_assert(noexcept(std::declval<P const &>().append(std::in_place_type<int>, 1)));
    static_assert(noexcept(std::declval<P &&>().append(std::in_place_type<int>, 1)));
    static_assert(noexcept(std::declval<P &>().append(1))); // deduced form
    SUCCEED();
  }
}

TEST_CASE("pack with immovable data", "[pack][immovable]")
{
  using fn::pack;

  struct ImmovableType {
    int value = 0;

    constexpr explicit ImmovableType(int i) noexcept : value(i) {}

    ImmovableType(ImmovableType const &) = delete;
    ImmovableType &operator=(ImmovableType const &) = delete;
    ImmovableType(ImmovableType &&) = delete;
    ImmovableType &operator=(ImmovableType &&) = delete;

    constexpr bool operator==(ImmovableType const &other) const noexcept { return value == other.value; }
  };

  using T = pack<ImmovableType, ImmovableType const, ImmovableType &, ImmovableType const &>;
  ImmovableType val1{15};
  ImmovableType const val2{92};
  T v{ImmovableType{3}, ImmovableType{14}, val1, val2};

  constexpr auto can_invoke = [](auto &&fn) constexpr { return requires { std::declval<T>().invoke(fn); }; };

  static_assert(can_invoke([](auto &&...) {})); // generic call
  static_assert(can_invoke([](ImmovableType const &, ImmovableType const &, ImmovableType const &,
                              ImmovableType const &) {})); // pass everything by const reference
  static_assert(can_invoke([](ImmovableType &&, ImmovableType const &&, ImmovableType &, ImmovableType const &) {
  }));                                                             // bind rvalues and lvalues
  static_assert(not can_invoke([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

  CHECK(v.invoke([](auto &&...args) noexcept -> int { return (0 + ... + args.value); }) == 3 + 14 + 15 + 92);
}

namespace {
struct Alef final {
  int value;
};
struct Bet final {
  int value;
};
struct Gimel final {
  int value;
};
struct Heh final {
  int value;
};
struct Vav final {
  int value;
};
struct Zayn final {
  int value;
};
} // namespace

namespace {
constexpr auto join_witness = [](auto &&...v) -> int { return (0 + ... + v.value); };

// One join algebra, two layers walking the same shape grid: the TEMPLATE_TEST_CASE below runs the
// battery through each subject, in both constant and runtime evaluation.
struct join_via_optional final { // fn::detail::_join over engaged optionals - the monads' layer
  template <typename R, typename LH, typename RH> static constexpr auto join(LH const &lh, RH const &rh)
  {
    constexpr auto efn = [](auto &&...) { return std::nullopt; };
    auto const r = fn::detail::_join<fn::optional>(fn::optional<LH>{lh}, fn::optional<RH>{rh}, efn);
    static_assert(std::is_same_v<decltype(r), fn::optional<R> const>);
    return r.value();
  }
};

struct join_via_operator final { // the public operator& over bare sums, packs and values
  template <typename R, typename LH, typename RH> static constexpr auto join(LH const &lh, RH const &rh)
  {
    auto const r = lh & rh;
    static_assert(std::is_same_v<decltype(r), R const>);
    return r;
  }
};

template <typename S> constexpr bool join_battery()
{
  using fn::pack;
  using fn::sum;

  bool ok = true;
  { // sum of packs join sum of scalars
    using R = sum<pack<Alef, Gimel, Heh>, pack<Alef, Gimel, Vav>, pack<Alef, Gimel, Zayn>, //
                  pack<Bet, Gimel, Heh>, pack<Bet, Gimel, Vav>, pack<Bet, Gimel, Zayn>>;
    auto const r = S::template join<R>(sum<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       sum<Heh, Vav, Zayn>{Vav{15}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14 + 15;
  }
  {                                                                     // sum of packs join sum of packs
    using R = sum<pack<Alef, Gimel, Heh, Zayn>, pack<Alef, Gimel, Vav>, //
                  pack<Bet, Gimel, Heh, Zayn>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(sum<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       sum<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14 + 15;
  }
  { // sum of scalars join sum of scalars
    using R = sum<pack<Alef, Heh>, pack<Alef, Vav>, pack<Alef, Zayn>, pack<Bet, Heh>, pack<Bet, Vav>, pack<Bet, Zayn>,
                  pack<Gimel, Heh>, pack<Gimel, Vav>, pack<Gimel, Zayn>>;
    auto const r = S::template join<R>(sum<Alef, Bet, Gimel>{Gimel{3}}, sum<Heh, Vav, Zayn>{Vav{14}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14;
  }
  {                                                                             // sum of scalars join sum of packs
    using R = sum<pack<Alef, Heh, Zayn>, pack<Alef, Vav>, pack<Bet, Heh, Zayn>, //
                  pack<Bet, Vav>, pack<Gimel, Heh, Zayn>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(sum<Alef, Bet, Gimel>{Gimel{3}}, sum<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14;
  }
  { // sum of packs join scalar
    using R = sum<pack<Alef, Gimel, Vav>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(sum<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}}, Vav{15});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14 + 15;
  }
  { // sum of packs join pack
    using R = sum<pack<Alef, Gimel, Vav>, pack<Bet, Gimel, Vav>>;
    auto const r = S::template join<R>(sum<pack<Alef, Gimel>, pack<Bet, Gimel>>{pack{Alef{3}, Gimel{14}}},
                                       pack<Vav>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14 + 15;
  }
  { // sum of scalars join scalar
    using R = sum<pack<Alef, Vav>, pack<Bet, Vav>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(sum<Alef, Bet, Gimel>{Gimel{3}}, Vav{14});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14;
  }
  { // sum of scalars join pack
    using R = sum<pack<Alef, Vav>, pack<Bet, Vav>, pack<Gimel, Vav>>;
    auto const r = S::template join<R>(sum<Alef, Bet, Gimel>{Gimel{3}}, pack<Vav>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14;
  }
  { // pack join sum of scalars
    using R = sum<pack<Alef, Gimel, Heh>, pack<Alef, Gimel, Vav>, pack<Alef, Gimel, Zayn>>;
    auto const r = S::template join<R>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}}, sum<Heh, Vav, Zayn>{Vav{15}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14 + 15;
  }
  { // pack join sum of packs
    using R = sum<pack<Alef, Gimel, Heh, Zayn>, pack<Alef, Gimel, Vav>>;
    auto const r = S::template join<R>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}},
                                       sum<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{15}}});
    ok = ok && r.template has_value<pack<Alef, Gimel, Vav>>() && r.invoke(join_witness) == 3 + 14 + 15;
  }
  { // pack join scalar
    auto const r = S::template join<pack<Alef, Gimel, Vav>>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}}, Vav{15});
    ok = ok && r.invoke(join_witness) == 3 + 14 + 15;
  }
  { // pack join pack
    auto const r = S::template join<pack<Alef, Gimel, Vav>>(pack<Alef, Gimel>{pack{Alef{3}, Gimel{14}}},
                                                            pack<Vav>{pack{Vav{15}}});
    ok = ok && r.invoke(join_witness) == 3 + 14 + 15;
  }
  return ok;
}

// A bare value can sit on the LEFT of the monad-level join only - operator& has no overload for it,
// so these four shapes belong to join_via_optional alone.
constexpr bool join_value_lhs_battery()
{
  using fn::pack;
  using fn::sum;
  using S = join_via_optional;

  bool ok = true;
  { // scalar join sum of scalars
    using R = sum<pack<Alef, Heh>, pack<Alef, Vav>, pack<Alef, Zayn>>;
    auto const r = S::join<R>(Alef{3}, sum<Heh, Vav, Zayn>{Vav{14}});
    ok = ok && r.template has_value<pack<Alef, Vav>>() && r.invoke(join_witness) == 3 + 14;
  }
  { // scalar join sum of packs
    using R = sum<pack<Alef, Heh, Zayn>, pack<Alef, Vav>>;
    auto const r = S::join<R>(Alef{3}, sum<pack<Heh, Zayn>, pack<Vav>>{pack{Vav{14}}});
    ok = ok && r.template has_value<pack<Alef, Vav>>() && r.invoke(join_witness) == 3 + 14;
  }
  { // scalar join scalar
    auto const r = S::join<pack<Alef, Vav>>(Alef{3}, Vav{14});
    ok = ok && r.invoke(join_witness) == 3 + 14;
  }
  { // scalar join pack
    auto const r = S::join<pack<Alef, Vav>>(Alef{3}, pack<Vav>{pack{Vav{14}}});
    ok = ok && r.invoke(join_witness) == 3 + 14;
  }
  return ok;
}
} // namespace

TEMPLATE_TEST_CASE("join of sums, packs and values", "[pack][sum][detail][optional][operator_and]", join_via_optional,
                   join_via_operator)
{
  static_assert(join_battery<TestType>());
  REQUIRE(join_battery<TestType>());
}

TEST_CASE("detail::_join with value operands", "[detail][pack][sum][optional]")
{
  static_assert(join_value_lhs_battery());
  REQUIRE(join_value_lhs_battery());
}

TEST_CASE("operator &", "[pack][sum][operator_and]")
{
  constexpr auto r1 = fn::as_sum(12) & 3 & 2.5 & fn::pack{0.5, true}
                      & fn::sum_for<bool, int, fn::pack<double, int>>(fn::pack{1.5, 12});
  static_assert(std::is_same_v<                                     //
                decltype(r1),                                       //
                fn::sum_for<                                        //
                    fn::pack<int, int, double, double, bool, bool>, //
                    fn::pack<int, int, double, double, bool, int>,  //
                    fn::pack<int, int, double, double, bool, double, int>> const>);
  static_assert(r1.invoke([](auto &&...args) -> double { return (1 * ... * static_cast<double>(args)); })
                == 12. * 3 * 2.5 * 0.5 * 1 * 1.5 * 12);

  constexpr auto r2
      = fn::identity(12, 3, 2.5, fn::pack{0.5, true}, fn::sum_for<bool, int, fn::pack<double, int>>(fn::pack{1.5, 12}));
  static_assert(std::is_same_v<                                     //
                decltype(r2),                                       //
                fn::sum_for<                                        //
                    fn::pack<int, int, double, double, bool, bool>, //
                    fn::pack<int, int, double, double, bool, int>,  //
                    fn::pack<int, int, double, double, bool, double, int>> const>);
  static_assert(r2.invoke([](auto &&...args) -> double { return (1 * ... * static_cast<double>(args)); })
                == 12. * 3 * 2.5 * 0.5 * 1 * 1.5 * 12);

  SECTION("noexcept")
  {
    // This operator& carries no noexcept specifier, so it never over-promises - in contrast with
    // optional's and expected's, which are unconditionally noexcept although the join copies the
    // operands' values into the result pack (#279).
    static_assert(not noexcept(std::declval<fn::pack<int> &>() & 2));
    static_assert(not noexcept(std::declval<fn::sum<int> &>() & 2));
    SUCCEED();
  }
}

namespace {
template <typename P, std::size_t I>
concept can_get = requires(P p) { fn::get<I>(static_cast<P &&>(p)); };
} // namespace

TEST_CASE("pack get and tuple protocol", "[pack][get][tuple]")
{
  using fn::get;
  using fn::pack;

  // tuple_size / tuple_element
  static_assert(std::tuple_size_v<pack<int, double, A>> == 3);
  static_assert(std::tuple_size_v<pack<>> == 0);
  static_assert(std::same_as<std::tuple_element_t<0, pack<int, double, A>>, int>);
  static_assert(std::same_as<std::tuple_element_t<1, pack<int, double, A>>, double>);
  static_assert(std::same_as<std::tuple_element_t<2, pack<int, double, A>>, A>);
  // the library's tuple_element<I, const T> propagates const onto value elements
  static_assert(std::same_as<std::tuple_element_t<0, pack<int, double, A> const>, int const>);

  // get value categories mirror what invoke passes
  pack<int, double> p{2, 4};
  static_assert(std::same_as<decltype(get<0>(p)), int &>);
  static_assert(std::same_as<decltype(get<1>(p)), double &>);
  static_assert(std::same_as<decltype(get<0>(std::as_const(p))), int const &>);
  static_assert(std::same_as<decltype(get<0>(std::move(p))), int &&>);
  static_assert(std::same_as<decltype(get<0>(std::move(std::as_const(p)))), int const &&>);

  CHECK(get<0>(p) == 2);
  CHECK(get<1>(p) == 4.0);
  get<0>(p) = 7;
  CHECK(get<0>(p) == 7);

  // accurate, unlike the members that relocate elements: get only forms a reference
  static_assert(noexcept(get<0>(p)));
  static_assert(noexcept(get<0>(std::move(p))));
  static_assert(noexcept(get<0>(std::declval<pack<Throwing> &>())));

  // out-of-range index is not viable (SFINAE-clean, not a hard error)
  static_assert(can_get<pack<int, double> &, 0>);
  static_assert(can_get<pack<int, double> &, 1>);
  static_assert(not can_get<pack<int, double> &, 2>);
  static_assert(not can_get<pack<> &, 0>);

  // reference-holding element: a non-const pack yields the stored reference
  int x = 11;
  pack<int &> r{x};
  static_assert(std::same_as<std::tuple_element_t<0, pack<int &>>, int &>);
  static_assert(std::same_as<decltype(get<0>(r)), int &>);
  get<0>(r) = 12;
  CHECK(x == 12);

  // const pack of a reference: fn propagates const through the reference (unlike
  // std::tuple<int&>, whose reference member is immune to container const), so
  // get and tuple_element agree on int const& here.
  static_assert(std::same_as<std::tuple_element_t<0, pack<int &> const>, int const &>);
  static_assert(std::same_as<decltype(get<0>(std::as_const(r))), int const &>);
  static_assert(std::same_as<decltype(get<0>(std::move(std::as_const(r)))), int const &>);
  CHECK(get<0>(std::as_const(r)) == 12);
  // a const structured binding over the const pack observes the const-propagated reference
  {
    auto const &[e0] = std::as_const(r);
    static_assert(std::same_as<decltype(e0), int const &>);
    CHECK(e0 == 12);
  }

  // structured bindings over an lvalue pack alias the elements
  auto &[a0, a1] = p;
  CHECK(a0 == 7);
  CHECK(a1 == 4.0);
  a0 = 9;
  CHECK(get<0>(p) == 9);

  // structured bindings over an rvalue pack move the elements out
  auto [b0, b1] = std::move(p);
  static_assert(std::same_as<decltype(b0), int>);
  static_assert(std::same_as<decltype(b1), double>);
  CHECK(b0 == 9);
  CHECK(b1 == 4.0);

  // generic idiom: unqualified get with std::get also in scope resolves to fn::get by ADL
  {
    using std::get;
    pack<int, double> q{3, 1.5};
    CHECK(get<0>(q) == 3);
    CHECK(get<1>(q) == 1.5);
  }

  // constexpr twins
  static_assert(std::tuple_size_v<pack<int, double>> == 2);
  static_assert([] {
    pack<int, double> q{5, 2.5};
    auto &[c0, c1] = q;
    c0 = 6;
    return get<0>(q) == 6 && get<1>(q) == 2.5;
  }());
  static_assert([] {
    int y = 3;
    pack<int &> s{y};
    get<0>(s) = 4;
    return y == 4;
  }());
  static_assert([] {
    int y = 5;
    pack<int &> s{y};
    // const-propagation twin: get on the const pack yields int const&, and
    // tuple_element agrees, so the binding is read-only yet still aliases y.
    static_assert(std::same_as<decltype(get<0>(std::as_const(s))), int const &>);
    static_assert(std::same_as<std::tuple_element_t<0, pack<int &> const>, int const &>);
    auto const &[e0] = std::as_const(s);
    return e0 == 5;
  }());
}
