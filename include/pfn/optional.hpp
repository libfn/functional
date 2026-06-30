// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_OPTIONAL
#define INCLUDE_PFN_OPTIONAL

#include <cassert>
#include <memory>
#include <optional> // For anything that is not std::optional or std::make_optional
#include <type_traits>
#include <utility>

#ifdef FWD
#pragma push_macro("FWD")
#define INCLUDE_PFN_OPTIONAL__POP_FWD
#undef FWD
#endif

// Also defined in fn/detail/fwd_macro.hpp but pfn/* headers are standalone
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#ifdef ASSERT
#pragma push_macro("ASSERT")
#define INCLUDE_PFN_OPTIONAL__POP_ASSERT
#undef ASSERT
#endif

// LIBFN_ASSERT is a customization point for the user
#ifdef LIBFN_ASSERT
#define ASSERT(...) LIBFN_ASSERT(__VA_ARGS__)
#else
#define ASSERT(...) assert((__VA_ARGS__) == true)
#endif

namespace pfn {

// [optional.optional], class template optional
template <class T> class optional; // partially freestanding

// [optional.optional.ref], partial specialization of optional for lvalue reference types
template <class T> class optional<T &>; // partially freestanding

template <class T>
concept _is_derived_from_optional = requires(T const &t) { // exposition only
  []<class U>(optional<U> const &) {}(t);
};

// [optional.relops], relational operators
template <class T, class U> constexpr bool operator==(optional<T> const &, optional<U> const &);
template <class T, class U> constexpr bool operator!=(optional<T> const &, optional<U> const &);
template <class T, class U> constexpr bool operator<(optional<T> const &, optional<U> const &);
template <class T, class U> constexpr bool operator>(optional<T> const &, optional<U> const &);
template <class T, class U> constexpr bool operator<=(optional<T> const &, optional<U> const &);
template <class T, class U> constexpr bool operator>=(optional<T> const &, optional<U> const &);
template <class T, ::std::three_way_comparable_with<T> U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &, optional<U> const &);

// [optional.nullops], comparison with nullopt
template <class T> constexpr bool operator==(optional<T> const &, ::std::nullopt_t) noexcept;
template <class T> constexpr ::std::strong_ordering operator<=>(optional<T> const &, ::std::nullopt_t) noexcept;

// [optional.comp.with.t], comparison with T
template <class T, class U> constexpr bool operator==(optional<T> const &, U const &);
template <class T, class U> constexpr bool operator==(T const &, optional<U> const &);
template <class T, class U> constexpr bool operator!=(optional<T> const &, U const &);
template <class T, class U> constexpr bool operator!=(T const &, optional<U> const &);
template <class T, class U> constexpr bool operator<(optional<T> const &, U const &);
template <class T, class U> constexpr bool operator<(T const &, optional<U> const &);
template <class T, class U> constexpr bool operator>(optional<T> const &, U const &);
template <class T, class U> constexpr bool operator>(T const &, optional<U> const &);
template <class T, class U> constexpr bool operator<=(optional<T> const &, U const &);
template <class T, class U> constexpr bool operator<=(T const &, optional<U> const &);
template <class T, class U> constexpr bool operator>=(optional<T> const &, U const &);
template <class T, class U> constexpr bool operator>=(T const &, optional<U> const &);
template <class T, class U>
  requires(!_is_derived_from_optional<U>) && ::std::three_way_comparable_with<T, U>
constexpr ::std::compare_three_way_result_t<T, U> operator<=>(optional<T> const &, U const &);

// [optional.specalg], specialized algorithms
template <class T> constexpr void swap(optional<T> &, optional<T> &) noexcept(true); // TODO noexcept

template <class T> constexpr optional<::std::decay_t<T>> make_optional(T &&);
template <class T, class... Args> constexpr optional<T> make_optional(Args &&...args);
template <class T, class U, class... Args>
constexpr optional<T> make_optional(::std::initializer_list<U> il, Args &&...args);

namespace detail {

// Internal union wrapper for optional's value / no-value storage. This mirrors the
// strategy of `pfn::detail::_expected_union_t<void, E>` with the value and "empty"
// roles reversed: the engaged value lives in `v_`, and `e_` is a trivial placeholder
// for the disengaged state, so the union always has an active member (required for
// constexpr use). Copy/move ctors are defaulted iff `T` is trivially copy/move
// constructible; the destructor is defaulted iff `T` is trivially destructible.
template <class T> union _optional_union_t {
  // _dummy_t placeholder, so the union always has an active member
  struct _dummy_t final {
    constexpr _dummy_t() noexcept = default;
  };

  using _value_t = T;
  T v_;
  _dummy_t e_; // disengaged-state placeholder

  template <typename S>
  constexpr explicit _optional_union_t(bool s, S &&src) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype((FWD(src).v_))>)
  {
    if (s)
      ::std::construct_at(::std::addressof(v_), FWD(src).v_);
    else
      ::std::construct_at(::std::addressof(e_));
  }

  constexpr _optional_union_t(_optional_union_t const &) = delete;
  constexpr _optional_union_t(_optional_union_t const &) //
    requires(::std::is_trivially_copy_constructible_v<T>)
  = default;
  constexpr _optional_union_t(_optional_union_t &&) = delete;
  constexpr _optional_union_t(_optional_union_t &&) //
    requires(::std::is_trivially_move_constructible_v<T>)
  = default;
  constexpr _optional_union_t &operator=(_optional_union_t const &) = delete;
  constexpr _optional_union_t &operator=(_optional_union_t &&) = delete;

  template <class... Args>
  constexpr explicit _optional_union_t(::std::in_place_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>)
    requires ::std::is_constructible_v<T, Args...>
      : v_(FWD(a)...)
  {
  }
  constexpr explicit _optional_union_t(::std::nullopt_t /*ignored*/) noexcept : e_{} {}

  constexpr ~_optional_union_t() noexcept
    requires(::std::is_trivially_destructible_v<T>)
  = default;
  constexpr ~_optional_union_t() noexcept {}

  // reinit, mirroring [expected.void.assign]'s direct construction: one of New/Old is
  // always the trivial `_dummy_t` (value <-> empty transition), so no strong-exception
  // snapshot of the old member is needed.
  template <typename New, typename Old, typename... Args>
  static constexpr void _reinit(New *newp, Old *oldp, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<New, Args...>)
  {
    if constexpr (::std::is_same_v<New, _dummy_t>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp); // Never throws, since New is _dummy_t
    } else if constexpr (::std::is_nothrow_constructible_v<New, Args...>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp, ::std::forward<Args>(args)...); // Never throws
    } else {
      ::std::destroy_at(oldp);
      try {
        ::std::construct_at(newp, ::std::forward<Args>(args)...);
      } catch (...) {
        ::std::construct_at(oldp); // Never throws, since Old is _dummy_t
        throw;
      }
    }
  }
};

// optional<T&> stores a single pointer: nullptr encodes the disengaged state, so no
// discriminant or placeholder is needed (T* is trivially copyable/destructible).
template <class T> union _optional_union_t<T &> {
  T *v_ = nullptr;
};

} // namespace detail

template <class T> class optional {
public:
  using value_type = T;

  // [optional.ctor], constructors
  constexpr optional() noexcept;
  constexpr optional(::std::nullopt_t) noexcept;
  constexpr optional(optional const &);
  constexpr optional(optional &&) noexcept(true); // TODO noexcept
  template <class... Args> constexpr explicit optional(::std::in_place_t, Args &&...);
  template <class U, class... Args>
  constexpr explicit optional(::std::in_place_t, ::std::initializer_list<U>, Args &&...);
  template <class U = ::std::remove_cv_t<T>> constexpr explicit(true) optional(U &&); // TODO explicit
  template <class U> constexpr explicit(true) optional(optional<U> const &);          // TODO explicit
  template <class U> constexpr explicit(true) optional(optional<U> &&);               // TODO explicit

  // [optional.dtor], destructor
  constexpr ~optional();

  // [optional.assign], assignment
  constexpr optional &operator=(::std::nullopt_t) noexcept;
  constexpr optional &operator=(optional const &);
  constexpr optional &operator=(optional &&) noexcept(true); // TODO noexcept
  template <class U = ::std::remove_cv_t<T>> constexpr optional &operator=(U &&);
  template <class U> constexpr optional &operator=(optional<U> const &);
  template <class U> constexpr optional &operator=(optional<U> &&);
  template <class... Args> constexpr T &emplace(Args &&...);
  template <class U, class... Args> constexpr T &emplace(::std::initializer_list<U>, Args &&...);

  // [optional.swap], swap
  constexpr void swap(optional &) noexcept(true); // TODO noexcept

  // [optional.observe], observers
  constexpr T const *operator->() const noexcept;
  constexpr T *operator->() noexcept;
  constexpr T const &operator*() const & noexcept;
  constexpr T &operator*() & noexcept;
  constexpr T &&operator*() && noexcept;
  constexpr T const &&operator*() const && noexcept;
  constexpr explicit operator bool() const noexcept;
  constexpr bool has_value() const noexcept;
  constexpr T const &value() const &;   // freestanding-deleted
  constexpr T &value() &;               // freestanding-deleted
  constexpr T &&value() &&;             // freestanding-deleted
  constexpr T const &&value() const &&; // freestanding-deleted
  template <class U = ::std::remove_cv_t<T>> constexpr T value_or(U &&) const &;
  template <class U = ::std::remove_cv_t<T>> constexpr T value_or(U &&) &&;

  // [optional.monadic], monadic operations
  template <class F> constexpr auto and_then(F &&f) &;
  template <class F> constexpr auto and_then(F &&f) &&;
  template <class F> constexpr auto and_then(F &&f) const &;
  template <class F> constexpr auto and_then(F &&f) const &&;
  template <class F> constexpr auto transform(F &&f) &;
  template <class F> constexpr auto transform(F &&f) &&;
  template <class F> constexpr auto transform(F &&f) const &;
  template <class F> constexpr auto transform(F &&f) const &&;
  template <class F> constexpr optional or_else(F &&f) &&;
  template <class F> constexpr optional or_else(F &&f) const &;

  // [optional.mod], modifiers
  constexpr void reset() noexcept;

private:
  union {
    ::std::remove_cv_t<T> val; // exposition only
  };
};

template <class T> optional(T) -> optional<T>;

template <class T> class optional<T &> {
public:
  using value_type = T;

public:
  // [optional.ref.ctor], constructors
  constexpr optional() noexcept = default;
  constexpr optional(::std::nullopt_t) noexcept : optional() {}
  constexpr optional(optional const &rhs) noexcept = default;

  template <class Arg> constexpr explicit optional(::std::in_place_t, Arg &&arg);
  template <class U> constexpr explicit(true) optional(U &&u) noexcept(true);                  // TODO explicit/noexcept
  template <class U> constexpr explicit(true) optional(optional<U> &rhs) noexcept(true);       // TODO explicit/noexcept
  template <class U> constexpr explicit(true) optional(optional<U> const &rhs) noexcept(true); // TODO explicit/noexcept
  template <class U> constexpr explicit(true) optional(optional<U> &&rhs) noexcept(true);      // TODO explicit/noexcept
  template <class U>
  constexpr explicit(true) optional(optional<U> const &&rhs) noexcept(true); // TODO explicit/noexcept

  constexpr ~optional() = default;

  // [optional.ref.assign], assignment
  constexpr optional &operator=(::std::nullopt_t) noexcept;
  constexpr optional &operator=(optional const &rhs) noexcept = default;

  template <class U> constexpr T &emplace(U &&u) noexcept(true); // TODO noexcept

  // [optional.ref.swap], swap
  constexpr void swap(optional &rhs) noexcept;

  // [optional.ref.observe], observers
  constexpr T *operator->() const noexcept;
  constexpr T &operator*() const noexcept;
  constexpr explicit operator bool() const noexcept;
  constexpr bool has_value() const noexcept;
  constexpr T &value() const; // freestanding-deleted
  template <class U = ::std::remove_cv_t<T>> constexpr ::std::remove_cv_t<T> value_or(U &&u) const;

  // [optional.ref.monadic], monadic operations
  template <class F> constexpr auto and_then(F &&f) const;
  template <class F> constexpr optional<::std::invoke_result_t<F, T &>> transform(F &&f) const;
  template <class F> constexpr optional or_else(F &&f) const;

  // [optional.ref.mod], modifiers
  constexpr void reset() noexcept;

private:
  T *val = nullptr; // exposition only

  // [optional.ref.expos], exposition only helper functions
  template <class U> constexpr void _convert_ref_init_val(U &&u); // exposition only
};

} // namespace pfn

namespace std {
// [optional.hash], hash support
template <class T> struct hash<::pfn::optional<T>>;
} // namespace std

#undef ASSERT

#ifdef INCLUDE_PFN_OPTIONAL__POP_ASSERT
#pragma pop_macro("ASSERT")
#endif

#undef FWD

#ifdef INCLUDE_PFN_OPTIONAL__POP_FWD
#pragma pop_macro("FWD")
#endif

#endif // INCLUDE_PFN_OPTIONAL
