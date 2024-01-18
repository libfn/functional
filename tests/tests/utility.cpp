// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include "functional/utility.hpp"

#include <catch2/catch_all.hpp>

#include <expected>
#include <optional>

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

static_assert(some_expected<std::expected<int, bool>>);
static_assert(some_expected<std::expected<int, bool> const>);
static_assert(some_expected<std::expected<int, bool> &>);
static_assert(some_expected<std::expected<int, bool> const &>);
static_assert(some_expected<std::expected<int, bool> &&>);
static_assert(some_expected<std::expected<int, bool> const &&>);

static_assert(some_optional<std::optional<int>>);
static_assert(some_optional<std::optional<int> const>);
static_assert(some_optional<std::optional<int> &>);
static_assert(some_optional<std::optional<int> const &>);
static_assert(some_optional<std::optional<int> &&>);
static_assert(some_optional<std::optional<int> const &&>);

static_assert(some_monadic_type<std::expected<int, bool>>);
static_assert(some_monadic_type<std::expected<int, bool> const>);
static_assert(some_monadic_type<std::expected<int, bool> &>);
static_assert(some_monadic_type<std::expected<int, bool> const &>);
static_assert(some_monadic_type<std::expected<int, bool> &&>);
static_assert(some_monadic_type<std::expected<int, bool> const &&>);
static_assert(some_monadic_type<std::optional<int>>);
static_assert(some_monadic_type<std::optional<int> const>);
static_assert(some_monadic_type<std::optional<int> &>);
static_assert(some_monadic_type<std::optional<int> const &>);
static_assert(some_monadic_type<std::optional<int> &&>);
static_assert(some_monadic_type<std::optional<int> const &&>);
} // namespace fn

TEST_CASE("closure", "[closure]")
{
  using fn::closure;

  struct A final {
    int v = 0;
  };
  using T = closure<int, int const, int &, int const &>;
  int val1 = 15;
  int const val2 = 92;
  T v{3, 14, val1, val2};

  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](auto &&...) {})); // generic call
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, auto &&...) {})); // also generic call
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, int, int, int, int) {})); // pass everything by value
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A const &, int const &, int const &, int const &, int const &) {
  })); // pass everything by const reference
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, int, int, int &, int const &) {})); // bind lvalues
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A &&, int &&, int const &&, int &, int const &) {
  })); // bind rvalues and lvalues
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A const, int const, int const, int const &, int const &) {
  })); // pass values or lvalues promoted to const

  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A &, auto &&...) {})); // cannot bind rvalue to lvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, int &, auto &&...) {})); // cannot bind rvalue to lvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, int, int, int &&, int) {})); // cannot bind lvalue to rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, int, int, int, int &&) {
  })); // cannot bind const lvalue to rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A, int, int, int, int const &&) {
  })); // cannot bind const lvalue to const rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](A &&, int, int &&, int, int) {
  })); // cannot bind const rvalue to non-const rvalue reference
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](int, auto &&...) {})); // bad type
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](auto, auto, auto, auto) {})); // bad arity
  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn, A{}); };
  }([](auto, auto, auto, auto, auto, auto) {})); // bad arity

  CHECK(T::invoke(v,
                  [](auto... args) noexcept -> int { return (0 + ... + args); })
        == 3 + 14 + 15 + 92);
  CHECK(T::invoke(
            v, [](A, auto... args) noexcept -> int { return (0 + ... + args); },
            A{})
        == 3 + 14 + 15 + 92);

  A a;
  constexpr auto fn1 = [](A &dest, auto... args) noexcept -> A & {
    dest.v = (0 + ... + args);
    return dest;
  };
  CHECK(T::invoke(v, fn1, a).v == 3 + 14 + 15 + 92);
  static_assert(std::is_same_v<decltype(T::invoke(v, fn1, a)), A &>);

  constexpr auto fn2
      = [](A &&dest, auto &&...) noexcept -> A && { return std::move(dest); };
  static_assert(
      std::is_same_v<decltype(T::invoke(v, fn2, std::move(a))), A &&>);

  constexpr auto fn3 = [](A &&dest, auto &&...) noexcept -> A { return dest; };
  static_assert(std::is_same_v<decltype(T::invoke(v, fn3, std::move(a))), A>);
}

TEST_CASE("closure with immovable data", "[closure][immovable]")
{
  using fn::closure;

  struct ImmovableType {
    int value = 0;

    constexpr explicit ImmovableType(int i) noexcept : value(i) {}

    ImmovableType(ImmovableType const &) = delete;
    ImmovableType &operator=(ImmovableType const &) = delete;
    ImmovableType(ImmovableType &&) = delete;
    ImmovableType &operator=(ImmovableType &&) = delete;

    constexpr bool operator==(ImmovableType const &other) const noexcept
    {
      return value == other.value;
    }
  };

  using T = closure<ImmovableType, ImmovableType const, ImmovableType &,
                    ImmovableType const &>;
  ImmovableType val1{15};
  ImmovableType const val2{92};
  T v{ImmovableType{3}, ImmovableType{14}, val1, val2};

  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn); };
  }([](auto &&...) {})); // generic call
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn); };
  }([](ImmovableType const &, ImmovableType const &, ImmovableType const &,
       ImmovableType const &) {})); // pass everything by const reference
  static_assert([&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn); };
  }([](ImmovableType &&, ImmovableType const &&, ImmovableType &,
       ImmovableType const &) {})); // bind rvalues and lvalues

  static_assert(not [&](auto &&fn) constexpr {
    return requires { T::invoke(v, fn); };
  }([](ImmovableType, auto &&...) {})); // cannot pass immovable by value

  CHECK(T::invoke(v,
                  [](auto &&...args) noexcept -> int {
                    return (0 + ... + args.value);
                  })
        == 3 + 14 + 15 + 92);
}
