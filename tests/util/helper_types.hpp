// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#include <concepts>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <auto V> struct helper_t {
  static inline int witness = 0;
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

  ~helper_t() noexcept = default;

  bool operator==(helper_t const &) const noexcept = default;

  // Assignment operators will multiply witness by a prime
  helper_t &operator=(helper_t &o) noexcept
  {
    v = o.v;
    witness *= from_lval;
    return *this;
  }

  helper_t &operator=(helper_t const &o) noexcept
  {
    v = o.v;
    witness *= from_lval_const;
    return *this;
  }

  helper_t &operator=(helper_t &&o) noexcept
  {
    v = o.v;
    witness *= from_rval;
    return *this;
  }

  helper_t &operator=(helper_t const &&o) noexcept
  {
    v = o.v;
    witness *= from_rval_const;
    return *this;
  }

  // Every constructor will increase witness, including copy/move
  helper_t(helper_t &&o) noexcept : v(o.v) { witness *= from_rval; }
  helper_t(helper_t const &o) noexcept : v(o.v) { witness *= from_lval_const; }

  helper_t(std::integral auto... a) noexcept
    requires(sizeof...(a) > 0) // intentionally implicit when sizeof...(a) == 1
      : v((1 * ... * a))
  {
    witness += v;
  }

  template <typename T> //
  explicit helper_t(std::integral_constant<T, V>) noexcept : v(static_cast<int>(T::value))
  {
    witness += v;
  }

  // Potentially throwing constructor
  helper_t(std::initializer_list<double> list, std::integral auto... a) noexcept(false) : v(init(list, a...)) //
  {
    witness += v;
  }

  // ... and the actual exception being thrown
  static int init(std::initializer_list<double> l, auto &&...a) noexcept(false)
  {
    double ret = (1 * ... * a);
    for (auto d : l) {
      if (d == 0.0)
        throw std::runtime_error("invalid input");
      ret *= d;
    }
    return static_cast<int>(ret);
  }
};

// Swap will also multiply witness by a prime
template <auto V> void swap(helper_t<V> &l, helper_t<V> &r)
{
  std::swap(l.v, r.v);
  helper_t<V>::witness *= helper_t<V>::swapped;
}

using helper = helper_t<0>;
