// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <compare>
#include <concepts>
#include <exception>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <int V> struct helper_t {
  static inline int state = 0;

  int v = {};

  // Use prime numbers to record Foo states in witness
  enum {
    from_lval = 53, //
    from_lval_const = 59,
    from_rval = 61,
    from_rval_const = 67,
    swapped = 97
  };

  // No default constructor
  helper_t() = delete;

  constexpr ~helper_t() noexcept {};

  constexpr bool operator==(helper_t const &) const noexcept = default;

  // Assignment operators will multiply witness by a prime
  constexpr helper_t &operator=(helper_t &o) noexcept
  {
    v = o.v;
    v *= from_lval;
    return *this;
  }

  constexpr helper_t &operator=(helper_t const &o) noexcept
  {
    v = o.v;
    v *= from_lval_const;
    return *this;
  }

  constexpr helper_t &operator=(helper_t &&o) noexcept
  {
    v = o.v;
    v *= from_rval;
    return *this;
  }

  constexpr helper_t &operator=(helper_t const &&o) noexcept
  {
    v = o.v;
    v *= from_rval_const;
    return *this;
  }

  // See table below
  constexpr helper_t(helper_t &o) noexcept : v(o.v) { v *= from_lval; }
  constexpr helper_t(helper_t const &o) noexcept(V < 2 || V >= 4) : v(o.v)
  {
    v *= from_lval_const;
    if constexpr (V >= 2 && V < 4) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }
  constexpr helper_t(helper_t &&o) noexcept(V < 3 || V >= 5)
    requires(V < 30)
      : v(o.v)
  {
    v *= from_rval;
    if constexpr (V >= 3 && V < 5) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }
  helper_t(helper_t &&o) noexcept(V < 33 || V >= 35)
    requires(V >= 30)
      : v(o.v)
  {
    v *= from_rval;
    state += v;
    if constexpr (V >= 33 && V < 35) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }
  constexpr helper_t(helper_t const &&o) noexcept : v(o.v) { v *= from_rval_const; }

  // The intent of non-constexpr constructors is to make sure that they are never optimized away,
  // thus ensuring that any code which relies on them in tests will show up in coverage reports.
  helper_t(std::integral auto... a) noexcept(V >= 8)
    requires(sizeof...(a) > 0) // intentionally implicit when sizeof...(a) == 1
      : v((1 * ... * a))
  {
    if constexpr (V < 8) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
    state += v;
  }

  helper_t(std::initializer_list<double> list) noexcept(true) : v(init(list)) { state += v; }

  // Potentially throwing constructor
  constexpr helper_t(std::initializer_list<double> list, std::integral auto... a) noexcept(true)
    requires(sizeof...(a) > 0)
      : v(init(list, a...)) //
  {
  }

  // ... and the actual exception being thrown
  static constexpr int init(std::initializer_list<double> l, auto &&...a) noexcept
  {
    double ret = (1 * ... * a);
    for (auto d : l) {
      ret *= d;
    }
    return static_cast<int>(ret);
  }

  // Disable comparison operators; compare .v instead
  friend bool operator==(helper_t, std::integral auto) = delete;
  friend std::strong_ordering operator<=>(helper_t, helper_t) = delete;
};

//    helper_t<V>
// V  is_nothrow_copy_constructible  is_nothrow_move_constructible
// 0  1                              1
// 1  1                              1
// 2  0                              1
// 3  0                              0
// 4  1                              0
// 5  1                              1
static_assert(std::is_nothrow_copy_constructible_v<helper_t<0>>);
static_assert(std::is_nothrow_move_constructible_v<helper_t<0>>);
static_assert(std::is_nothrow_copy_constructible_v<helper_t<1>>);
static_assert(std::is_nothrow_move_constructible_v<helper_t<1>>);
static_assert(not std::is_nothrow_copy_constructible_v<helper_t<2>>);
static_assert(std::is_nothrow_move_constructible_v<helper_t<2>>);
static_assert(not std::is_nothrow_copy_constructible_v<helper_t<3>>);
static_assert(not std::is_nothrow_move_constructible_v<helper_t<3>>);
static_assert(std::is_nothrow_copy_constructible_v<helper_t<4>>);
static_assert(not std::is_nothrow_move_constructible_v<helper_t<4>>);
static_assert(std::is_nothrow_copy_constructible_v<helper_t<5>>);
static_assert(std::is_nothrow_move_constructible_v<helper_t<5>>);

// Swap will also multiply witness by a prime
template <auto V> constexpr void swap(helper_t<V> &l, helper_t<V> &r)
{
  std::swap(l.v, r.v);
  l.v *= l.swapped;
  r.v *= r.swapped;
}

using helper = helper_t<0>;
