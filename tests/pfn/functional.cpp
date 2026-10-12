// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "catch2/catch_test_macros.hpp"
#ifndef PFN_TEST_VALIDATION
#include <pfn/functional.hpp>
using pfn::invoke_r;
#define PFN_INVOKE_R_TESTS 1
#else
#include <functional>
#if defined(__cpp_lib_invoke_r) && __cpp_lib_invoke_r >= 202106L
using std::invoke_r;
#define PFN_INVOKE_R_TESTS 1
#endif
#endif

#include <type_traits>
#include <utility>

#ifndef PFN_INVOKE_R_TESTS
TEST_CASE("invoke_r unavailable in this standard library", "[functional][polyfill][invoke_r]")
{
  SUCCEED("std::invoke_r not provided by this standard library");
}
#else

namespace {

constexpr int add(int a, int b) noexcept { return a + b; }
int may_throw(int a) { return a + 1; }

struct S {
  int x;
  constexpr int twice() const noexcept { return x * 2; }
  int may_throw_member() { return x + 1; }
};

struct Convertible {
  int v;
  constexpr operator int() const noexcept { return v; }
};

constexpr Convertible make(int v) noexcept { return Convertible{v}; }

} // namespace

TEST_CASE("invoke_r free function", "[functional][polyfill][invoke_r]")
{
  static_assert(invoke_r<int>(add, 2, 3) == 5);
  static_assert(noexcept(invoke_r<int>(add, 2, 3)));
  CHECK(invoke_r<long>(add, 2, 3) == 5L);

  static_assert(not noexcept(invoke_r<int>(may_throw, 1)));
  CHECK(invoke_r<int>(may_throw, 1) == 2);
}

TEST_CASE("invoke_r member access", "[functional][polyfill][invoke_r]")
{
  constexpr S s{10};
  static_assert(invoke_r<int>(&S::twice, s) == 20);
  static_assert(invoke_r<long>(&S::x, s) == 10L);
  static_assert(noexcept(invoke_r<int>(&S::twice, s)));

  S m{3};
  CHECK(invoke_r<int>(&S::may_throw_member, m) == 4);
  static_assert(not noexcept(invoke_r<int>(&S::may_throw_member, std::declval<S &>())));
}

TEST_CASE("invoke_r implicit conversion to R", "[functional][polyfill][invoke_r]")
{
  static_assert(invoke_r<int>(make, 7) == 7);
  static_assert(std::is_same_v<int, decltype(invoke_r<int>(make, 7))>);
}

TEST_CASE("invoke_r void R discards", "[functional][polyfill][invoke_r]")
{
  int side = 0;
  auto const fn = [&side](int n) {
    side += n;
    return side;
  };

  static_assert(std::is_same_v<void, decltype(invoke_r<void>(fn, 1))>);
  invoke_r<void>(fn, 5);
  CHECK(side == 5);
  invoke_r<void>(fn, 2);
  CHECK(side == 7);
}

namespace {
constexpr int value_of(S const &s) noexcept { return s.x; }
constexpr long const &long_of(long const &l) noexcept { return l; }

template <typename R, typename F, typename... Args>
concept can_invoke_r
    = requires(F &&f, Args &&...args) { invoke_r<R>(static_cast<F &&>(f), static_cast<Args &&>(args)...); };
} // namespace

TEST_CASE("invoke_r reference result", "[functional][polyfill][invoke_r]")
{
  static constexpr S s{10};

  SECTION("binds the referent")
  {
    static_assert(&invoke_r<int const &>(&S::x, s) == &s.x);
    CHECK(&invoke_r<int const &>(&S::x, s) == &s.x);
    constexpr auto xvalue = [] {
      constexpr auto binds = [](int const &&r) { return &r == &s.x; };
      return binds(invoke_r<int const &&>(&S::x, std::move(s)));
    };
    static_assert(xvalue());
    CHECK(xvalue());

    static constexpr long l = 3;
    static_assert(&invoke_r<long const &>(long_of, l) == &l);
    CHECK(&invoke_r<long const &>(long_of, l) == &l);
    static_assert(invoke_r<int>(value_of, s) == 10);
    CHECK(invoke_r<int>(value_of, s) == 10);
  }

  SECTION("binding a temporary")
  {
    // [func.require]/2: INVOKE<R> is ill-formed where the result would bind R to a temporary
    static_assert(can_invoke_r<int const &, decltype(&S::x), S const &>);
    static_assert(can_invoke_r<int const &, decltype(&S::x), S const &&>);
    static_assert(not can_invoke_r<int const &, decltype(value_of) &, S const &>);
    static_assert(not can_invoke_r<int const &, decltype(long_of) &, long const &>);
    static_assert(can_invoke_r<long const &, decltype(long_of) &, long const &>);
    static_assert(can_invoke_r<int, decltype(value_of) &, S const &>);
    static_assert(can_invoke_r<void, decltype(value_of) &, S const &>);
    SUCCEED();
  }
}

TEST_CASE("invoke_r SFINAE-friendly via concept", "[functional][polyfill][invoke_r]")
{
  static_assert(std::is_invocable_r_v<int, decltype(add) &, int, int>);
  static_assert(std::is_invocable_r_v<long, decltype(add) &, int, int>);
  static_assert(std::is_invocable_r_v<void, decltype(add) &, int, int>);
  static_assert(not std::is_invocable_r_v<int *, decltype(add) &, int, int>);
}

#endif // PFN_INVOKE_R_TESTS
