// Copyright (c) 2024 Bronek Kozicki, Alex Kremer
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/utility.hpp"
#include "static_check.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>

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

namespace {
struct A final {
  int v = 0;
};
} // namespace

TEST_CASE("pack", "[pack]")
{
  using fn::pack;

  using T = pack<int, int const, int &, int const &>;
  int val1 = 15;
  int const val2 = 92;
  T v{3, 14, val1, val2};
  CHECK(v.size() == 4);

  using is = static_check::bind<decltype([](auto &&fn) constexpr {
    return requires { std::declval<T>().invoke(fn, A{}); };
  })>;

  static_assert(is::invocable([](auto &&...) {}));            // generic call
  static_assert(is::invocable([](A, auto &&...) {}));         // also generic call
  static_assert(is::invocable([](A, int, int, int, int) {})); // pass everything by value
  static_assert(is::invocable(
      [](A const &, int const &, int const &, int const &, int const &) {})); // pass everything by const reference
  static_assert(is::invocable([](A, int, int, int &, int const &) {}));       // bind lvalues
  static_assert(is::invocable([](A &&, int &&, int const &&, int &, int const &) {})); // bind rvalues and lvalues
  static_assert(is::invocable(
      [](A const, int const, int const, int const &, int const &) {})); // pass values or lvalues promoted to const

  static_assert(is::not_invocable([](A &, auto &&...) {}));          // cannot bind rvalue to lvalue reference
  static_assert(is::not_invocable([](A, int &, auto &&...) {}));     // cannot bind rvalue to lvalue reference
  static_assert(is::not_invocable([](A, int, int, int &&, int) {})); // cannot bind lvalue to rvalue reference
  static_assert(is::not_invocable([](A, int, int, int, int &&) {})); // cannot bind const lvalue to rvalue reference
  static_assert(
      is::not_invocable([](A, int, int, int, int const &&) {})); // cannot bind const lvalue to const rvalue reference
  static_assert(
      is::not_invocable([](A &&, int, int &&, int, int) {})); // cannot bind const rvalue to non-const rvalue reference
  static_assert(is::not_invocable([](int, auto &&...) {}));   // bad type
  static_assert(is::not_invocable([](auto, auto, auto, auto) {}));             // bad arity
  static_assert(is::not_invocable([](auto, auto, auto, auto, auto, auto) {})); // bad arity

  CHECK(v.invoke([](auto... args) noexcept -> int { return (0 + ... + args); }) == 3 + 14 + 15 + 92);
  CHECK(v.invoke([](A, auto... args) noexcept -> int { return (0 + ... + args); }, A{}) == 3 + 14 + 15 + 92);

  A a;
  constexpr auto fn1 = [](A &dest, auto... args) noexcept -> A & {
    dest.v = (0 + ... + args);
    return dest;
  };
  CHECK(v.invoke(fn1, a).v == 3 + 14 + 15 + 92);
  static_assert(std::is_same_v<decltype(v.invoke(fn1, a)), A &>);

  constexpr auto fn2 = [](A &&dest, auto &&...) noexcept -> A && { return std::move(dest); };
  static_assert(std::is_same_v<decltype(v.invoke(fn2, std::move(a))), A &&>);

  constexpr auto fn3 = [](A &&dest, auto &&...) noexcept -> A { return dest; };
  static_assert(std::is_same_v<decltype(v.invoke(fn3, std::move(a))), A>);

  static_assert(pack<>{}.size() == 0);

  static_assert(std::same_as<decltype(fn::pack{}), fn::pack<>>);
  static_assert(std::same_as<decltype(fn::pack{12}), fn::pack<int>>);
  static_assert(std::same_as<decltype(fn::pack{a}), fn::pack<A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, a}), fn::pack<int, A &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::as_const(a)}), fn::pack<int, A const &>>);
  static_assert(std::same_as<decltype(fn::pack{12, std::move(a)}), fn::pack<int, A>>);
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

  static_assert(T::size() == 3);
  constexpr auto check = [](int i, std::string_view s, A a, B const &b) noexcept -> bool {
    return i == 12 && s == std::string("bar") && a.v == 42 && b.v == 30;
  };

  WHEN("explicit type selection")
  {
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B>, 5, 6)), T::append_type<B>>);
    static_assert(std::same_as<T::append_type<B>, pack<int, std::string_view, A, B>>);
    static_assert(decltype(s.append(std::in_place_type<B>, 5, 6))::size() == 4);

    constexpr C c1{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B const &>, c1)), T::append_type<B const &>>);
    static_assert(std::same_as<T::append_type<B const &>, pack<int, std::string_view, A, B const &>>);

    C c2{};
    static_assert(std::same_as<decltype(s.append(std::in_place_type<B &>, c2)), T::append_type<B &>>);
    static_assert(std::same_as<T::append_type<B &>, pack<int, std::string_view, A, B &>>);

    WHEN("constructor takes parameters")
    {
      CHECK(s.append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(T{12, "bar", 42}.append(std::in_place_type<B>, 5, 6).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<B>, 5, 6).invoke(check));
    }

    WHEN("default constructor")
    {
      CHECK(s.append(std::in_place_type<C>).invoke(check));
      CHECK(std::as_const(s).append(std::in_place_type<C>).invoke(check));
      CHECK(T{12, "bar", 42}.append(std::in_place_type<C>).invoke(check));
      CHECK(std::move(std::as_const(s)).append(std::in_place_type<C>).invoke(check));
    }
  }

  WHEN("deduced type")
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
    CHECK(T{12, "bar", 42}.append(B{30}).invoke(check));
    CHECK(std::move(std::as_const(s)).append(B{30}).invoke(check));
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

  using is
      = static_check::bind<decltype([](auto &&fn) constexpr { return requires { std::declval<T>().invoke(fn); }; })>;

  static_assert(is::invocable([](auto &&...) {})); // generic call
  static_assert(is::invocable([](ImmovableType const &, ImmovableType const &, ImmovableType const &,
                                 ImmovableType const &) {})); // pass everything by const reference
  static_assert(is::invocable([](ImmovableType &&, ImmovableType const &&, ImmovableType &, ImmovableType const &) {
  }));                                                                // bind rvalues and lvalues
  static_assert(is::not_invocable([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

  CHECK(v.invoke([](auto &&...args) noexcept -> int { return (0 + ... + args.value); }) == 3 + 14 + 15 + 92);
}

TEST_CASE("constexpr pack", "[pack][constexpr]")
{
  constexpr fn::pack<int, int> v2{3, 14};
  constexpr auto r2 = v2.invoke([](auto &&...args) constexpr noexcept -> int { return (0 + ... + args); });
  static_assert(r2 == 3 + 14);
  SUCCEED();
}

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
