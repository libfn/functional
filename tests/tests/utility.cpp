// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/utility.hpp"
#include "static_check.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

using namespace util;

namespace fn {
// clang-format off
static_assert(std::is_same_v<as_value_t<int>,          int>);
static_assert(std::is_same_v<as_value_t<int const>,    int const>);
static_assert(std::is_same_v<as_value_t<int &&>,       int>);
static_assert(std::is_same_v<as_value_t<int const &&>, int const>);
static_assert(std::is_same_v<as_value_t<int &>,        int &>);
static_assert(std::is_same_v<as_value_t<int const &>,  int const &>);

static_assert(std::is_same_v<as_value_t<std::nullopt_t>,          std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const>,    std::nullopt_t const>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t &>,        std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const &>,  std::nullopt_t const>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t &&>,       std::nullopt_t>);
static_assert(std::is_same_v<as_value_t<std::nullopt_t const &&>, std::nullopt_t const>);

static_assert(std::is_same_v<apply_const_lvalue_t<float,          int>,    int>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const,    int>,    int const>);
static_assert(std::is_same_v<apply_const_lvalue_t<float,          int &>,  int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const,    int &>,  int const &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float,          int &&>, int &&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const,    int &&>, int const &&>);

static_assert(std::is_same_v<apply_const_lvalue_t<float &,        int>,    int&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &,  int>,    int const&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &,        int &>,  int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &,  int &>,  int const &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &,        int &&>, int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &,  int &&>, int const &>);

static_assert(std::is_same_v<apply_const_lvalue_t<float &&,       int>,    int>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &&, int>,    int const>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &&,       int &>,  int &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &&, int &>,  int const &>);
static_assert(std::is_same_v<apply_const_lvalue_t<float &&,       int &&>, int &&>);
static_assert(std::is_same_v<apply_const_lvalue_t<float const &&, int &&>, int const &&>);

static_assert(std::is_same_v<decltype(apply_const_lvalue<float>         (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const>   (std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float>         (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const>   (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float>         (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const>   (std::declval<int &&>())), int const &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &>       (std::declval<int>())),    int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &> (std::declval<int>())),    int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &>       (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &> (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &>       (std::declval<int &&>())), int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &> (std::declval<int &&>())), int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &&>      (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &&>(std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &&>      (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &&>(std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float &&>      (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const_lvalue<float const &&>(std::declval<int &&>())), int const &&>);
// clang-format on
} // namespace fn

TEST_CASE("overload", "[overload]")
{
  using is = static_check::bind<decltype([](auto &&fn) constexpr {
    return requires {
      fn();
      fn(1);
    };
  })>;

  // example use
  static_assert(is::invocable(fn::overload{[](auto...) {}}));       // generic
  static_assert(is::invocable(fn::overload{[]() {}, [](auto) {}})); // generic with fixed arity
  static_assert(is::invocable(fn::overload{[]() {}, [](int) {}}));  // fixed type
  static_assert(is::invocable(fn::overload{[]() {}, [](bool) {}})); // built-in conversion
  static_assert(is::invocable(
      fn::overload{[]() -> void {}, [](int i) -> std::string { return std::to_string(i); }})); // different return types

  static_assert(is::not_invocable(fn::overload{[]() {}}));    // missing int overload
  static_assert(is::not_invocable(fn::overload{[](int) {}})); // missing void overload

  struct A {
    constexpr auto operator()(int i) const noexcept -> int { return i + 1; }
  };

  // check template deduction guides
  static_assert(std::same_as<decltype(fn::overload{std::declval<A>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A &>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A &&>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A const>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A const &>()}), fn::overload<A>>);
  static_assert(std::same_as<decltype(fn::overload{std::declval<A const &&>()}), fn::overload<A>>);

  A a1 = {};
  CHECK(fn::overload{a1}(1) == 2);
  constexpr A a2 = {};
  static_assert(fn::overload{a2}(2) == 3);
  static_assert(fn::overload{A{}}(3) == 4);
}

TEST_CASE("make lift")
{
  WHEN("aggregate constructor")
  {
    constexpr auto a = fn::make<std::array<int, 2>>(3, 5);
    static_assert(std::is_same_v<decltype(a), std::array<int, 2> const>);
    static_assert(a[0] == 3 && a[1] == 5);
  }

  WHEN("aggregate class")
  {
    struct A {
      int const i;
    };
    constexpr auto a = fn::make<A>(12);
    static_assert(std::is_same_v<decltype(a), A const>);
    static_assert(a.i == 12);
  }
}
