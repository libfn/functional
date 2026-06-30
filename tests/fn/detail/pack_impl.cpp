// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

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
concept can_invoke = requires(P p, Fn fn, Args &&...args) { P::_invoke(p, fn, static_cast<Args &&>(args)...); };

template <typename P, typename T, typename... Args>
concept can_append = requires(P p, Args &&...args) { P::template _append<T>(p, static_cast<Args &&>(args)...); };

} // namespace

TEST_CASE("pack_impl size and aggregate construction", "[pack_impl]")
{
  using fn::detail::pack_impl;

  using P0 = pack_impl<std::index_sequence<>>;
  static_assert(P0::size == 0);
  static_assert(std::is_trivially_default_constructible_v<P0>);
  static_assert(P0::_invoke(P0{}, []() { return 7; }) == 7);

  using P1 = pack_impl<std::index_sequence_for<int>, int>;
  static_assert(P1::size == 1);
  constexpr P1 p1{42};
  static_assert(P1::_invoke(p1, [](int v) { return v; }) == 42);

  using P3 = pack_impl<std::index_sequence_for<int, double, A>, int, double, A>;
  static_assert(P3::size == 3);
  constexpr P3 p3{3, 1.5, A{14}};
  static_assert(P3::_invoke(p3, [](int i, double d, A a) { return i + d + a.v; }) == 18.5);

  // duplicate types are permitted at distinct indices
  using PD = pack_impl<std::index_sequence_for<int, int>, int, int>;
  constexpr PD pd{7, 9};
  static_assert(PD::_invoke(pd, [](int a, int b) { return a * 100 + b; }) == 709);

  // non-copyable element constructs in place and is reachable
  using PN = pack_impl<std::index_sequence_for<NonCopyable>, NonCopyable>;
  constexpr PN pn{NonCopyable{99}};
  static_assert(PN::_invoke(pn, [](NonCopyable const &n) { return n.v; }) == 99);
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
  CHECK(P::_invoke(pm, [](int i, double) { return i; }) == 5);
  CHECK(P::_invoke(pm, [](int, double d) { return d; }) == 6.0);

  // SFINAE: non-invocable fn is rejected (wrong arity)
  static_assert(not can_swap_invoke<P, decltype([]() {})>);
  // SFINAE: non-invocable fn is rejected (incompatible argument types)
  static_assert(not can_swap_invoke<P, decltype([](int *, int *) {})>);
  // positive control
  static_assert(can_swap_invoke<P, decltype([](int, double) {})>);
}

TEST_CASE("pack_impl _invoke", "[pack_impl][invoke]")
{
  using fn::detail::pack_impl;
  using P = pack_impl<std::index_sequence_for<int, double>, int, double>;

  constexpr P p{2, 0.5};
  // _invoke calls fn(elements..., args...)
  static_assert(P::_invoke(p, [](int i, double d) { return i + d; }) == 2.5);
  static_assert(P::_invoke(p, [](int i, double d, int suffix) { return i + d + suffix; }, 100) == 102.5);
  static_assert(std::same_as<decltype(P::_invoke(p, [](int, double) -> long { return 0; })), long>);

  // const lvalue propagation: elements are passed as const lvalue references when pack is const
  static_assert(P::_invoke(p, [](int const &, double const &) { return true; }));

  // mutable access via non-const pack
  P pm{3, 4.0};
  P::_invoke(pm, [](int &i, double &d) {
    i = 5;
    d = 6.0;
  });
  CHECK(P::_invoke(pm, [](int i, double) { return i; }) == 5);
  CHECK(P::_invoke(pm, [](int, double d) { return d; }) == 6.0);

  // SFINAE: non-invocable fn is rejected (wrong arity)
  static_assert(not can_invoke<P, decltype([]() {})>);
  // SFINAE: non-invocable fn is rejected (incompatible argument types)
  static_assert(not can_invoke<P, decltype([](int *, int *) {})>);
  // positive control
  static_assert(can_invoke<P, decltype([](int, double) {})>);
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
  static_assert(P1::_invoke(p1, [](int v) { return v; }) == 7);

  constexpr auto p2 = P1::template _append<double>(p1, 1.25);
  static_assert(std::same_as<decltype(p2), P2 const>);
  static_assert(P2::_invoke(p2, [](int i, double d) { return i + d; }) == 8.25);

  // construction with multi-arg ctor
  using PA = pack_impl<std::index_sequence_for<A>, A>;
  constexpr auto pa = P0::template _append<A>(p0, 99);
  static_assert(std::same_as<decltype(pa), PA const>);
  static_assert(PA::_invoke(pa, [](A const &a) { return a.v; }) == 99);

  // SFINAE: incompatible ctor args are rejected (no `A(char const *)`)
  static_assert(not can_append<P0, A, char const *>);
  // SFINAE: too many ctor args are rejected (no 2-arg `int` ctor)
  static_assert(not can_append<P0, int, int, int>);
  // positive control: single-arg int ctor accepted
  static_assert(can_append<P0, int, int>);

  // append_type<T> alias resolves to ::fn::pack<Ts..., T>
  static_assert(std::same_as<P2::append_type<bool>, ::fn::pack<int, double, bool>>);
  static_assert(std::same_as<P0::append_type<int>, ::fn::pack<int>>);
}
