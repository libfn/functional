// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_TESTS_UTIL_HELPER_TYPES
#define INCLUDE_TESTS_UTIL_HELPER_TYPES

#include <compare>
#include <concepts>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

// Prime factors recording which special member ran: the witness value is multiplied by one of
// these, so a test can recover the exact copy/move/swap path from the factorization.
enum helper_witness {
  from_lval = 53, //
  from_lval_const = 59,
  from_rval = 61,
  from_rval_const = 67,
  swapped = 97
};

using helper_list_t = std::initializer_list<double>;

// Behaviour axes for the witness fixtures, combined with operator|. The default (regular == 0) is
// copyable, movable and every special member noexcept; each flag opts one operation into throwing
// (noexcept(false), throws when the resulting value is zero). runtime_move additionally makes the
// move constructor non-constexpr so it cannot be elided and is observable in `state` and coverage.
enum class prop : unsigned {
  regular = 0,
  throw_copy = 1U << 0,        // copy constructor
  throw_move = 1U << 1,        // move constructor
  throw_copy_assign = 1U << 2, // copy assignment
  throw_move_assign = 1U << 3, // move assignment
  throw_value = 1U << 4,       // value (integral arguments) constructor
  runtime_move = 1U << 5,      // non-constexpr move constructor; accumulates into `state`
};

constexpr prop operator|(prop a, prop b) noexcept
{
  using U = std::underlying_type_t<prop>;
  return static_cast<prop>(static_cast<U>(a) | static_cast<U>(b));
}

constexpr prop operator&(prop a, prop b) noexcept
{
  using U = std::underlying_type_t<prop>;
  return static_cast<prop>(static_cast<U>(a) & static_cast<U>(b));
}

// Common value storage and value constructors shared by the three fixtures; the copy/move behaviour
// (witnessed, deleted, or throwing) is layered on by the derived templates below.
template <prop F> struct helper_value_base {
  static constexpr bool throws_value = (F & prop::throw_value) != prop::regular;

  static inline int state = 0;
  int v = {};

  helper_value_base() = delete;
  constexpr ~helper_value_base() noexcept {}
  constexpr bool operator==(helper_value_base const &) const noexcept = default;

  // Non-constexpr on purpose: guarantees the constructor is emitted (never folded away) so a test
  // relying on it shows up in coverage, and lets `state` witness that construction actually ran.
  helper_value_base(std::integral auto... a) noexcept(not throws_value)
    requires(sizeof...(a) > 0) // intentionally implicit when sizeof...(a) == 1
      : v((1 * ... * a))
  {
    if constexpr (throws_value) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
    state += v;
  }

  helper_value_base(helper_list_t list) noexcept : v(init(list)) { state += v; }

  constexpr helper_value_base(helper_list_t list, std::integral auto... a) noexcept
    requires(sizeof...(a) > 0)
      : v(init(list, a...)) //
  {
  }

  static constexpr int init(helper_list_t l, auto &&...a) noexcept
  {
    double ret = (1 * ... * a);
    for (auto d : l) {
      ret *= d;
    }
    return static_cast<int>(ret);
  }

protected:
  struct raw_t {
    explicit raw_t() = default;
  };
  // Side-effect-free initialiser used by the witnessing copy/move constructors of the derived
  // fixtures, which must set `v` without running the value constructor's throw/`state` logic.
  constexpr helper_value_base(raw_t, int value) noexcept : v(value) {}
};

// Fully copyable and movable witness. Each special member multiplies `v` by its prime and,
// per the flags in F, may be noexcept(false) and throw on a zero result.
template <prop F> struct helper_t : helper_value_base<F> {
  using base = helper_value_base<F>;
  static constexpr bool throws_copy = (F & prop::throw_copy) != prop::regular;
  static constexpr bool throws_move = (F & prop::throw_move) != prop::regular;
  static constexpr bool throws_copy_assign = (F & prop::throw_copy_assign) != prop::regular;
  static constexpr bool throws_move_assign = (F & prop::throw_move_assign) != prop::regular;

  using base::base;
  using base::v;

  constexpr helper_t(helper_t &o) noexcept : base(typename base::raw_t{}, o.v) { v *= from_lval; }

  constexpr helper_t(helper_t const &o) noexcept(not throws_copy) : base(typename base::raw_t{}, o.v)
  {
    v *= from_lval_const;
    if constexpr (throws_copy) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }

  constexpr helper_t(helper_t &&o) noexcept(not throws_move)
    requires((F & prop::runtime_move) == prop::regular)
      : base(typename base::raw_t{}, o.v)
  {
    v *= from_rval;
    if constexpr (throws_move) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }

  helper_t(helper_t &&o) noexcept(not throws_move)
    requires((F & prop::runtime_move) != prop::regular)
      : base(typename base::raw_t{}, o.v)
  {
    v *= from_rval;
    base::state += v;
    if constexpr (throws_move) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }

  constexpr helper_t(helper_t const &&o) noexcept : base(typename base::raw_t{}, o.v) { v *= from_rval_const; }

  constexpr helper_t &operator=(helper_t &o) noexcept(not throws_copy_assign)
  {
    if constexpr (throws_copy_assign) {
      if (o.v == 0)
        throw std::runtime_error("invalid input");
    }
    v = o.v;
    v *= from_lval;
    return *this;
  }

  constexpr helper_t &operator=(helper_t const &o) noexcept(not throws_copy_assign)
  {
    if constexpr (throws_copy_assign) {
      if (o.v == 0)
        throw std::runtime_error("invalid input");
    }
    v = o.v;
    v *= from_lval_const;
    return *this;
  }

  constexpr helper_t &operator=(helper_t &&o) noexcept(not throws_move_assign)
  {
    if constexpr (throws_move_assign) {
      if (o.v == 0)
        throw std::runtime_error("invalid input");
    }
    v = o.v;
    v *= from_rval;
    return *this;
  }

  constexpr helper_t &operator=(helper_t const &&o) noexcept(not throws_move_assign)
  {
    if constexpr (throws_move_assign) {
      if (o.v == 0)
        throw std::runtime_error("invalid input");
    }
    v = o.v;
    v *= from_rval_const;
    return *this;
  }

  constexpr bool operator==(helper_t const &) const noexcept = default;

  // Disable comparison operators; compare .v instead
  friend bool operator==(helper_t, std::integral auto) = delete;
  friend std::strong_ordering operator<=>(helper_t, helper_t) = delete;
};

// Move-only witness: copy construction/assignment deleted, moves witnessed. runtime_move does not
// apply (the move constructor is always constexpr, matching the fixture's only historical use).
template <prop F> struct helper_move_only_t : helper_value_base<F> {
  using base = helper_value_base<F>;
  static constexpr bool throws_move = (F & prop::throw_move) != prop::regular;
  static constexpr bool throws_move_assign = (F & prop::throw_move_assign) != prop::regular;

  using base::base;
  using base::v;

  helper_move_only_t(helper_move_only_t const &) = delete;
  helper_move_only_t &operator=(helper_move_only_t const &) = delete;

  constexpr helper_move_only_t(helper_move_only_t &&o) noexcept(not throws_move) : base(typename base::raw_t{}, o.v)
  {
    v *= from_rval;
    if constexpr (throws_move) {
      if (v == 0)
        throw std::runtime_error("invalid input");
    }
  }

  constexpr helper_move_only_t(helper_move_only_t const &&o) noexcept : base(typename base::raw_t{}, o.v)
  {
    v *= from_rval_const;
  }

  constexpr helper_move_only_t &operator=(helper_move_only_t &&o) noexcept(not throws_move_assign)
  {
    if constexpr (throws_move_assign) {
      if (o.v == 0)
        throw std::runtime_error("invalid input");
    }
    v = o.v;
    v *= from_rval;
    return *this;
  }

  constexpr bool operator==(helper_move_only_t const &) const noexcept = default;
};

// Immovable witness: all copy/move deleted; only in-place constructible (from a value) and observable.
template <prop F> struct helper_immovable_t : helper_value_base<F> {
  using base = helper_value_base<F>;

  using base::base;
  using base::v;

  helper_immovable_t(helper_immovable_t const &) = delete;
  helper_immovable_t(helper_immovable_t &&) = delete;
  helper_immovable_t &operator=(helper_immovable_t const &) = delete;
  helper_immovable_t &operator=(helper_immovable_t &&) = delete;

  constexpr bool operator==(helper_immovable_t const &) const noexcept = default;
};

// Swap multiplies each witness by its prime.
template <prop F> constexpr void swap(helper_t<F> &l, helper_t<F> &r)
{
  std::swap(l.v, r.v);
  l.v *= swapped;
  r.v *= swapped;
}

using helper = helper_t<prop::throw_value>;
using helper_move_only = helper_move_only_t<prop::throw_value>;
using helper_immovable = helper_immovable_t<prop::throw_value>;

static_assert(std::is_nothrow_copy_constructible_v<helper>);
static_assert(std::is_nothrow_move_constructible_v<helper>);
static_assert(std::is_nothrow_copy_assignable_v<helper>);
static_assert(std::is_nothrow_move_assignable_v<helper>);

static_assert(not std::is_nothrow_copy_constructible_v<helper_t<prop::throw_copy>>);
static_assert(std::is_nothrow_move_constructible_v<helper_t<prop::throw_copy>>);
static_assert(std::is_nothrow_copy_constructible_v<helper_t<prop::throw_move>>);
static_assert(not std::is_nothrow_move_constructible_v<helper_t<prop::throw_move>>);
static_assert(not std::is_nothrow_copy_assignable_v<helper_t<prop::throw_copy_assign>>);
static_assert(std::is_nothrow_move_assignable_v<helper_t<prop::throw_copy_assign>>);
static_assert(std::is_nothrow_copy_assignable_v<helper_t<prop::throw_move_assign>>);
static_assert(not std::is_nothrow_move_assignable_v<helper_t<prop::throw_move_assign>>);

static_assert(not std::is_copy_constructible_v<helper_move_only>);
static_assert(std::is_nothrow_move_constructible_v<helper_move_only>);
static_assert(not std::is_nothrow_move_constructible_v<helper_move_only_t<prop::throw_move>>);
static_assert(not std::is_copy_assignable_v<helper_move_only>);
static_assert(std::is_nothrow_move_assignable_v<helper_move_only>);
static_assert(not std::is_nothrow_move_assignable_v<helper_move_only_t<prop::throw_move_assign>>);

static_assert(not std::is_copy_constructible_v<helper_immovable>);
static_assert(not std::is_move_constructible_v<helper_immovable>);
static_assert(not std::is_copy_assignable_v<helper_immovable>);
static_assert(not std::is_move_assignable_v<helper_immovable>);

#endif // INCLUDE_TESTS_UTIL_HELPER_TYPES
