// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <fn/utility.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <concepts>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>

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

namespace {

struct Agg final {
  int const i;
};

// Braces prefer an initializer_list constructor, so deleting it makes P unbraceable while leaving
// P(int) reachable through parentheses - which is precisely the case make's second overload exists
// for, and the only way to select it.
struct ParenOnly final {
  int i;

  constexpr ParenOnly(int v) noexcept : i(v) {}
  ParenOnly(std::initializer_list<int>) = delete;
};

// Type-keyed, and so dependent: a requires-expression written directly against a concrete type is a
// hard error when the requirement is invalid, rather than the false one wants.
template <typename T, typename... Args>
concept can_brace = requires(Args... args) { T{args...}; };
template <typename T, typename... Args>
concept can_paren = requires(Args... args) { T(args...); };
template <typename T, typename... Args>
concept can_make = requires(Args... args) { fn::make<T>(args...); };

} // namespace

TEST_CASE("overload", "[overload]")
{
  constexpr auto can_call = [](auto &&fn) constexpr {
    return requires {
      fn();
      fn(1);
    };
  };

  // example use
  static_assert(can_call(fn::overload{[](auto...) {}}));       // generic
  static_assert(can_call(fn::overload{[]() {}, [](auto) {}})); // generic with fixed arity
  static_assert(can_call(fn::overload{[]() {}, [](int) {}}));  // fixed type
  static_assert(can_call(fn::overload{[]() {}, [](bool) {}})); // built-in conversion
  static_assert(can_call(
      fn::overload{[]() -> void {}, [](int i) -> std::string { return std::to_string(i); }})); // different return types

  static_assert(not can_call(fn::overload{[]() {}}));    // missing int overload
  static_assert(not can_call(fn::overload{[](int) {}})); // missing void overload

  // noexcept follows the alternative that overload resolution actually picks, rather than being
  // flattened across them - the accuracy the verbs and sum do not manage
  constexpr auto mixed = fn::overload{[](int) noexcept -> int { return 1; }, //
                                      [](double) -> int { return 2; }};
  static_assert(noexcept(mixed(1)));
  static_assert(not noexcept(mixed(1.5)));
  CHECK(mixed(1) == 1);
  CHECK(mixed(1.5) == 2);

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

TEST_CASE("make lift", "[make]")
{
  SECTION("aggregate constructor")
  {
    constexpr auto a = fn::make<std::array<int, 2>>(3, 5);
    static_assert(std::is_same_v<decltype(a), std::array<int, 2> const>);
    static_assert(a[0] == 3 && a[1] == 5);

    auto const b = fn::make<std::array<int, 2>>(3, 5); // runtime twin
    CHECK(b[0] == 3);
    CHECK(b[1] == 5);
  }

  SECTION("aggregate class")
  {
    constexpr auto a = fn::make<Agg>(12);
    static_assert(std::is_same_v<decltype(a), Agg const>);
    static_assert(a.i == 12);

    CHECK(fn::make<Agg>(12).i == 12);
  }

  SECTION("paren fallback")
  {
    // The second overload takes over only where the first is not viable, which is the one thing the
    // pair of them is for - and ParenOnly is the case that separates them.
    static_assert(not can_brace<ParenOnly, int>);
    static_assert(can_paren<ParenOnly, int>);
    static_assert(can_make<ParenOnly, int>);

    constexpr auto p = fn::make<ParenOnly>(1);
    static_assert(std::is_same_v<decltype(p), ParenOnly const>);
    static_assert(p.i == 1);

    CHECK(fn::make<ParenOnly>(1).i == 1);
  }

  SECTION("constraints")
  {
    static_assert(can_make<Agg, int>);
    static_assert(not can_make<Agg, char const *>); // not constructible from it
    static_assert(not can_make<Agg, int, int>);     // too many initialisers

    // make carries no noexcept specifier, so it reports potentially-throwing even where the
    // construction cannot throw - the same converse accuracy gap as the as_sum and as_pack lifts.
    static_assert(std::is_nothrow_constructible_v<int, int>);
    static_assert(not noexcept(fn::make<int>(12))); // brace overload
    static_assert(std::is_nothrow_constructible_v<ParenOnly, int>);
    static_assert(not noexcept(fn::make<ParenOnly>(1))); // paren fallback overload

    SUCCEED();
  }
}
