// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <concepts>
#include <type_traits>

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

static_assert(std::is_same_v<apply_const_t<float,          int>,    int>);
static_assert(std::is_same_v<apply_const_t<float const,    int>,    int const>);
static_assert(std::is_same_v<apply_const_t<float,          int &>,  int &>);
static_assert(std::is_same_v<apply_const_t<float const,    int &>,  int const &>);
static_assert(std::is_same_v<apply_const_t<float,          int &&>, int &&>);
static_assert(std::is_same_v<apply_const_t<float const,    int &&>, int const &&>);

static_assert(std::is_same_v<apply_const_t<float &,        int>,    int>);
static_assert(std::is_same_v<apply_const_t<float const &,  int>,    int const>);
static_assert(std::is_same_v<apply_const_t<float &,        int &>,  int &>);
static_assert(std::is_same_v<apply_const_t<float const &,  int &>,  int const &>);
static_assert(std::is_same_v<apply_const_t<float &,        int &&>, int &&>);
static_assert(std::is_same_v<apply_const_t<float const &,  int &&>, int const &&>);

static_assert(std::is_same_v<apply_const_t<float &&,       int>,    int>);
static_assert(std::is_same_v<apply_const_t<float const &&, int>,    int const>);
static_assert(std::is_same_v<apply_const_t<float &&,       int &>,  int &>);
static_assert(std::is_same_v<apply_const_t<float const &&, int &>,  int const &>);
static_assert(std::is_same_v<apply_const_t<float &&,       int &&>, int &&>);
static_assert(std::is_same_v<apply_const_t<float const &&, int &&>, int const &&>);

static_assert(std::is_same_v<decltype(apply_const<float>         (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const>   (std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float>         (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const<float const>   (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const<float>         (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const>   (std::declval<int &&>())), int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &>       (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &> (std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &>       (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const<float const &> (std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const<float &>       (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &> (std::declval<int &&>())), int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &&>      (std::declval<int>())),    int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &&>(std::declval<int>())),    int const &&>);
static_assert(std::is_same_v<decltype(apply_const<float &&>      (std::declval<int &>())),  int &>);
static_assert(std::is_same_v<decltype(apply_const<float const &&>(std::declval<int &>())),  int const &>);
static_assert(std::is_same_v<decltype(apply_const<float &&>      (std::declval<int &&>())), int &&>);
static_assert(std::is_same_v<decltype(apply_const<float const &&>(std::declval<int &&>())), int const &&>);
// clang-format on
} // namespace fn

TEST_CASE("pack", "[pack]")
{
  using fn::pack;

  struct A final {
    int v = 0;
  };
  using T = pack<int, int const, int &, int const &>;
  int val1 = 15;
  int const val2 = 92;
  T v{3, 14, val1, val2};
  CHECK(v.size() == 4);

  static_assert(
      [&](auto &&fn) constexpr { return requires { v.invoke(fn, A{}); }; }([](auto &&...) {})); // generic call
  static_assert(
      [&](auto &&fn) constexpr { return requires { v.invoke(fn, A{}); }; }([](A, auto &&...) {})); // also generic call
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A, int, int, int, int) {})); // pass everything by value
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A const &, int const &, int const &, int const &, int const &) {})); // pass everything by const reference
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A, int, int, int &, int const &) {})); // bind lvalues
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A &&, int &&, int const &&, int &, int const &) {})); // bind rvalues and lvalues
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A const, int const, int const, int const &, int const &) {})); // pass values or lvalues promoted to const

  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A &, auto &&...) {})); // cannot bind rvalue to lvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A, int &, auto &&...) {})); // cannot bind rvalue to lvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A, int, int, int &&, int) {})); // cannot bind lvalue to rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A, int, int, int, int &&) {})); // cannot bind const lvalue to rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A, int, int, int, int const &&) {})); // cannot bind const lvalue to const rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](A &&, int, int &&, int, int) {})); // cannot bind const rvalue to non-const rvalue reference
  static_assert(
      not [&](auto &&fn) constexpr { return requires { v.invoke(fn, A{}); }; }([](int, auto &&...) {})); // bad type
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](auto, auto, auto, auto) {})); // bad arity
  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn, A{}); };
  }([](auto, auto, auto, auto, auto, auto) {})); // bad arity

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

  static_assert([&](auto &&fn) constexpr { return requires { v.invoke(fn); }; }([](auto &&...) {})); // generic call
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn); };
  }([](ImmovableType const &, ImmovableType const &, ImmovableType const &, ImmovableType const &) {
  })); // pass everything by const reference
  static_assert([&](auto &&fn) constexpr {
    return requires { v.invoke(fn); };
  }([](ImmovableType &&, ImmovableType const &&, ImmovableType &, ImmovableType const &) {
  })); // bind rvalues and lvalues

  static_assert(not [&](auto &&fn) constexpr {
    return requires { v.invoke(fn); };
  }([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

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
  // example use
  static_assert([](auto &&v) constexpr -> bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[](auto...) {}})); // generic
  static_assert([](auto &&v) constexpr -> bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[]() {}, [](auto) {}})); // generic with fixed arity
  static_assert([](auto &&v) constexpr -> bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[]() {}, [](int) {}})); // fixed type
  static_assert([](auto &&v) constexpr -> bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[]() {}, [](bool) {}})); // built-in conversion
  static_assert([](auto &&v) constexpr -> bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[]() -> void {}, [](int i) -> std::string { return std::to_string(i); }})); // different return types

  static_assert(not [](auto &&v) constexpr->bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[]() {}})); // missing int overload
  static_assert(not [](auto &&v) constexpr->bool {
    return requires {
      v();
      v(1);
    };
  }(fn::overload{[](int) {}})); // missing void overload

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
