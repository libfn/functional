// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/copack.hpp>
#include <fn/detail/pack_impl.hpp>

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace {

struct A final {
  int v;
  constexpr A(int i) noexcept : v(i) {}
};

struct NonCopyable final {
  int v;
  constexpr NonCopyable(int i) noexcept : v(i) {}
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;
  constexpr NonCopyable(NonCopyable &&) = default;
};

template <typename P, typename Fn, typename... Args>
concept can_swap_invoke
    = requires(P p, Fn fn, Args &&...args) { P::_swap_invoke(p, fn, static_cast<Args &&>(args)...); };

template <typename P, typename Fn, typename... Args>
concept can_invoke = requires(P p, Fn fn, Args &&...args) { P::_apply(p, fn, static_cast<Args &&>(args)...); };

template <typename P, typename T, typename... Args>
concept can_append = requires(P p, Args &&...args) { P::template _append<T>(p, static_cast<Args &&>(args)...); };

template <typename P, std::size_t I>
concept can_get = requires(P p) { std::remove_cvref_t<P>::template _get<I>(static_cast<P &&>(p)); };

} // namespace

TEST_CASE("pack_impl size and aggregate construction", "[pack_impl]")
{
  using fn::detail::pack_impl;

  using P0 = pack_impl<std::index_sequence<>>;
  static_assert(P0::size == 0);
  static_assert(std::is_trivially_default_constructible_v<P0>);
  static_assert(P0::_apply(P0{}, []() { return 7; }) == 7);

  using P1 = pack_impl<std::index_sequence_for<int>, int>;
  static_assert(P1::size == 1);
  constexpr P1 p1{42};
  static_assert(P1::_apply(p1, [](int v) { return v; }) == 42);

  using P3 = pack_impl<std::index_sequence_for<int, double, A>, int, double, A>;
  static_assert(P3::size == 3);
  constexpr P3 p3{3, 1.5, A{14}};
  static_assert(P3::_apply(p3, [](int i, double d, A a) { return i + d + a.v; }) == 18.5);

  // duplicate types are permitted at distinct indices
  using PD = pack_impl<std::index_sequence_for<int, int>, int, int>;
  constexpr PD pd{7, 9};
  static_assert(PD::_apply(pd, [](int a, int b) { return a * 100 + b; }) == 709);

  // non-copyable element constructs in place and is reachable
  using PN = pack_impl<std::index_sequence_for<NonCopyable>, NonCopyable>;
  constexpr PN pn{NonCopyable{99}};
  static_assert(PN::_apply(pn, [](NonCopyable const &n) { return n.v; }) == 99);

  SUCCEED();
}

TEST_CASE("pack_impl noexcept", "[pack_impl][noexcept]")
{
  using fn::detail::pack_impl;

  // An element whose copy and move can both throw, so that what the members promise can be compared
  // with what they actually do.
  struct Throwing final {
    int v;

    constexpr Throwing(int i) noexcept : v(i) {}
    constexpr Throwing(Throwing const &o) noexcept(false) : v(o.v) {}
    constexpr Throwing(Throwing &&o) noexcept(false) : v(o.v) {}
  };
  static_assert(not std::is_nothrow_copy_constructible_v<Throwing>);

  using P = pack_impl<std::index_sequence_for<int, double>, int, double>;
  using PT = pack_impl<std::index_sequence_for<Throwing>, Throwing>;

  constexpr auto throwing_fn = [](auto &&...) noexcept(false) -> int { return 0; };

  // both dispatchers weigh the callback they apply
  static_assert(not noexcept(P::_swap_invoke(std::declval<P const &>(), throwing_fn)));
  static_assert(not noexcept(P::_apply(std::declval<P const &>(), throwing_fn)));

  // _append weighs the element it constructs AND every element it relocates into the new pack: here
  // the appended int cannot throw, but the Throwing already in the pack must be moved across ...
  static_assert(not noexcept(PT::template _append<int>(std::declval<PT const &>(), 1)));
  // ... while constructing a Throwing from an int cannot throw, and nothing relocated can either
  static_assert(noexcept(P::template _append<Throwing>(std::declval<P const &>(), 1)));

  // _get's promise is accurate: it only forms a reference to an element, touching nothing.
  static_assert(noexcept(P::template _get<0>(std::declval<P const &>())));
  static_assert(noexcept(PT::template _get<0>(std::declval<PT &>())));

  SUCCEED();
}

TEST_CASE("pack_impl _swap_invoke", "[pack_impl][swap_invoke]")
{
  using fn::detail::pack_impl;
  using P = pack_impl<std::index_sequence_for<int, double>, int, double>;

  constexpr P p{2, 0.5};
  // _swap_invoke calls fn(args..., elements...)
  static_assert(P::_swap_invoke(p, [](int i, double d) { return i + d; }) == 2.5);
  static_assert(P::_swap_invoke(p, [](int prefix, int i, double d) { return prefix + i + d; }, 100) == 102.5);
  static_assert(
      P::_swap_invoke(p, [](char const *tag, int i, double d) { return tag[0] == 'x' && i == 2 && d == 0.5; }, "x"));
  static_assert(std::same_as<decltype(P::_swap_invoke(p, [](int, double) -> long { return 0; })), long>);

  // const lvalue propagation: elements are passed as const lvalue references when pack is const
  static_assert(P::_swap_invoke(p, [](int const &, double const &) { return true; }));

  // mutable access via non-const pack
  P pm{3, 4.0};
  P::_swap_invoke(pm, [](int &i, double &d) {
    i = 5;
    d = 6.0;
  });
  CHECK(P::_apply(pm, [](int i, double) { return i; }) == 5);
  CHECK(P::_apply(pm, [](int, double d) { return d; }) == 6.0);

  // SFINAE: non-applicable fn is rejected (wrong arity)
  static_assert(not can_swap_invoke<P, decltype([]() {})>);
  // SFINAE: non-applicable fn is rejected (incompatible argument types)
  static_assert(not can_swap_invoke<P, decltype([](int *, int *) {})>);
  // positive control
  static_assert(can_swap_invoke<P, decltype([](int, double) {})>);
}

TEST_CASE("pack_impl _apply", "[pack_impl][apply]")
{
  using fn::detail::pack_impl;
  using P = pack_impl<std::index_sequence_for<int, double>, int, double>;

  constexpr P p{2, 0.5};
  // _apply calls fn(elements..., args...)
  static_assert(P::_apply(p, [](int i, double d) { return i + d; }) == 2.5);
  static_assert(P::_apply(p, [](int i, double d, int suffix) { return i + d + suffix; }, 100) == 102.5);
  static_assert(std::same_as<decltype(P::_apply(p, [](int, double) -> long { return 0; })), long>);

  // const lvalue propagation: elements are passed as const lvalue references when pack is const
  static_assert(P::_apply(p, [](int const &, double const &) { return true; }));

  // mutable access via non-const pack
  P pm{3, 4.0};
  P::_apply(pm, [](int &i, double &d) {
    i = 5;
    d = 6.0;
  });
  CHECK(P::_apply(pm, [](int i, double) { return i; }) == 5);
  CHECK(P::_apply(pm, [](int, double d) { return d; }) == 6.0);

  // SFINAE: non-applicable fn is rejected (wrong arity)
  static_assert(not can_invoke<P, decltype([]() {})>);
  // SFINAE: non-applicable fn is rejected (incompatible argument types)
  static_assert(not can_invoke<P, decltype([](int *, int *) {})>);
  // positive control
  static_assert(can_invoke<P, decltype([](int, double) {})>);
}

TEST_CASE("pack_impl _get", "[pack_impl][get]")
{
  using fn::detail::pack_impl;
  using P = pack_impl<std::index_sequence_for<int, double>, int, double>;

  // value category of the pack is carried onto the returned reference
  P p{2, 4.0};
  static_assert(std::same_as<decltype(P::_get<0>(p)), int &>);
  static_assert(std::same_as<decltype(P::_get<1>(p)), double &>);
  static_assert(std::same_as<decltype(P::_get<0>(std::as_const(p))), int const &>);
  static_assert(std::same_as<decltype(P::_get<0>(std::move(p))), int &&>);
  static_assert(std::same_as<decltype(P::_get<0>(std::move(std::as_const(p)))), int const &&>);

  CHECK(P::_get<0>(p) == 2);
  CHECK(P::_get<1>(p) == 4.0);
  P::_get<0>(p) = 5;
  CHECK(P::_get<0>(p) == 5);

  // in-range indices are viable, out-of-range is not
  static_assert(can_get<P &, 0>);
  static_assert(can_get<P &, 1>);
  static_assert(not can_get<P &, 2>);

  // constexpr twin
  static_assert([] {
    P q{7, 1.5};
    P::_get<1>(q) = 3.5;
    return P::_get<0>(q) == 7 && P::_get<1>(q) == 3.5;
  }());
}

TEST_CASE("pack_impl _append and append_type", "[pack_impl][append][append_type]")
{
  using fn::detail::pack_impl;
  using P0 = pack_impl<std::index_sequence<>>;
  using P1 = pack_impl<std::index_sequence_for<int>, int>;
  using P2 = pack_impl<std::index_sequence_for<int, double>, int, double>;

  constexpr P0 p0{};
  constexpr auto p1 = P0::template _append<int>(p0, 7);
  static_assert(std::same_as<decltype(p1), P1 const>);
  static_assert(P1::_apply(p1, [](int v) { return v; }) == 7);

  constexpr auto p2 = P1::template _append<double>(p1, 1.25);
  static_assert(std::same_as<decltype(p2), P2 const>);
  static_assert(P2::_apply(p2, [](int i, double d) { return i + d; }) == 8.25);

  // construction with multi-arg ctor
  using PA = pack_impl<std::index_sequence_for<A>, A>;
  constexpr auto pa = P0::template _append<A>(p0, 99);
  static_assert(std::same_as<decltype(pa), PA const>);
  static_assert(PA::_apply(pa, [](A const &a) { return a.v; }) == 99);

  // SFINAE: incompatible ctor args are rejected (no `A(char const *)`)
  static_assert(not can_append<P0, A, char const *>);
  // SFINAE: too many ctor args are rejected (no 2-arg `int` ctor)
  static_assert(not can_append<P0, int, int, int>);
  // positive control: single-arg int ctor accepted
  static_assert(can_append<P0, int, int>);
  // SFINAE: a pack never holds a copack, so append_type<copack> names no type and the overload drops out
  static_assert(not can_append<P0, ::fn::copack<int>, ::fn::copack<int>>);

  // append_type<T> alias resolves to ::fn::pack<Ts..., T>
  static_assert(std::same_as<P2::append_type<bool>, ::fn::pack<int, double, bool>>);
  static_assert(std::same_as<P0::append_type<int>, ::fn::pack<int>>);
}
