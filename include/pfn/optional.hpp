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

template <typename> constexpr bool _is_optional_union = false;
template <typename T> constexpr bool _is_optional_union<_optional_union_t<T>> = true;

// Shared implementation base class for ::pfn::optional. Members are public since
// inheritance is private. Provides storage, constructors, destructors, and the
// non-converting assignment/emplace machinery; converting assignment is deferred
// alongside the converting constructors below.
template <class T> struct _optional_base {
  using _storage_t = _optional_union_t<T>;
  using _value_t = _storage_t::_value_t; // == T
  _storage_t storage_;
  bool set_;

  // Shared construction helper: selects the union's active member from `s`. Used by
  // the wrapper's copy/move and (later) the converting constructors.
  template <typename S>
  constexpr explicit _optional_base(bool s, S &&src)
    requires(_is_optional_union<::std::remove_cvref_t<S>>)
      : storage_(s, FWD(src)), set_(s)
  {
  }

  template <class... Args>
  constexpr explicit _optional_base(::std::in_place_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, Args...>
      : storage_(::std::in_place, FWD(a)...), set_(true)
  {
  }
  template <class U, class... Args>
  constexpr explicit _optional_base(::std::in_place_t /*ignored*/, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, ::std::initializer_list<U> &, Args...>
      : storage_(::std::in_place, il, FWD(a)...), set_(true)
  {
  }
  constexpr explicit _optional_base(::std::nullopt_t /*ignored*/) noexcept //
      : storage_(::std::nullopt), set_(false)
  {
  }

  constexpr _optional_base(_optional_base const &) = default;
  constexpr _optional_base(_optional_base &&) = default;

  constexpr ~_optional_base() //
    requires(::std::is_trivially_destructible_v<_value_t>)
  = default;
  constexpr ~_optional_base() //
    requires(not ::std::is_trivially_destructible_v<_value_t>)
  {
    if (set_)
      ::std::destroy_at(::std::addressof(storage_.v_));
  }

  // [optional.assign]: transitions to the disengaged state if currently engaged; never
  // throws, since _reinit's target is always the trivial `_dummy_t`. Shared by
  // operator=(nullopt_t) and emplace.
  constexpr void _reset() noexcept
  {
    if (set_) {
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_));
      set_ = false;
    }
  }

  // Assignment body shared by the public optional operator= overloads, which keep their
  // constraints/noexcept clauses and forward `s` here as an lvalue or rvalue.
  constexpr void _assign(auto &&s)
  {
    if (set_ && s.set_) {
      storage_.v_ = FWD(s).storage_.v_;
    } else if (set_) {
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_), FWD(s).storage_.e_);
      set_ = false;
    } else if (s.set_) {
      _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), FWD(s).storage_.v_);
      set_ = true;
    } else {
      storage_.e_ = FWD(s).storage_.e_;
    }
  }

  template <class... Args>
  constexpr T &emplace(Args &&...args) //
    requires ::std::is_constructible_v<T, Args...>
  {
    _reset();
    _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), FWD(args)...);
    set_ = true;
    return storage_.v_;
  }
  template <class U, class... Args>
  constexpr T &emplace(::std::initializer_list<U> il, Args &&...args) //
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
  {
    _reset();
    _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), il, FWD(args)...);
    set_ = true;
    return storage_.v_;
  }

  // [optional.observe], observers
  constexpr T const *operator->() const noexcept
  {
    ASSERT(set_); // LCOV_EXCL_LINE
    return ::std::addressof(storage_.v_);
  }
  constexpr T *operator->() noexcept
  {
    ASSERT(set_); // LCOV_EXCL_LINE
    return ::std::addressof(storage_.v_);
  }
  constexpr T const &operator*() const & noexcept { return *(this->operator->()); }
  constexpr T &operator*() & noexcept { return *(this->operator->()); }
  constexpr T const &&operator*() const && noexcept { return ::std::move(*(this->operator->())); }
  constexpr T &&operator*() && noexcept { return ::std::move(*(this->operator->())); }
  constexpr explicit operator bool() const noexcept { return set_; }
  constexpr bool has_value() const noexcept { return set_; }
};

// optional<T&> needs its own base: the referent is held directly as a pointer (nullptr
// encodes the disengaged state), so there is no value union, no discriminant flag, and
// every special member is implicitly trivial. The in_place ctor binds the reference by
// storing its address (full [optional.ref.ctor] constraints are deferred).
template <class T> struct _optional_base<T &> {
  using _value_t = T &;
  T *v_ = nullptr;

  constexpr explicit _optional_base(::std::nullopt_t /*ignored*/) noexcept {}

  template <class Arg>
  constexpr explicit _optional_base(::std::in_place_t /*ignored*/, Arg &&arg) noexcept //
      : v_(::std::addressof(arg))
  {
  }
};

} // namespace detail

template <class T> class optional : private detail::_optional_base<T> {
  using _base = detail::_optional_base<T>;

public:
  using value_type = T;

  // [optional.ctor], constructors
  constexpr optional() noexcept : _base(::std::nullopt) {}
  constexpr optional(::std::nullopt_t) noexcept : _base(::std::nullopt) {}

  constexpr optional(optional const &) = delete;
  constexpr optional(optional const &s)                   //
      noexcept(::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<T>)
  = default;
  constexpr optional(optional const &s)                   //
      noexcept(::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && not ::std::is_trivially_copy_constructible_v<T>)
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr optional(optional &&) noexcept
    requires(::std::is_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<T>)
  = default;
  constexpr optional(optional &&s) //
      noexcept(::std::is_nothrow_move_constructible_v<T>)
    requires(::std::is_move_constructible_v<T> && not ::std::is_trivially_move_constructible_v<T>)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  template <class... Args>
  constexpr explicit optional(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>) // extension
    requires ::std::is_constructible_v<T, Args...>
      : _base(::std::in_place, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit optional(::std::in_place_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)  // extension
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
      : _base(::std::in_place, il, FWD(a)...)
  {
  }

  // TODO converting constructors (U&&, optional<U> const&, optional<U>&&): deferred until the
  // _can_convert* base traits + [optional.ctor] constraints land. Left undeclared on purpose —
  // an unconstrained optional(U&&) would hijack the copy/move ctors for non-const lvalues.

  // [optional.dtor], destructor
  constexpr ~optional() = default;

  // [optional.assign], assignment
  constexpr optional &operator=(::std::nullopt_t) noexcept
  {
    this->_reset();
    return *this;
  }
  constexpr optional &operator=(optional const &) = delete;
  constexpr optional &operator=(optional const &s)                                                  //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<T>)
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr optional &operator=(optional &&s) // NOSONAR cpp:S5018 standard mandated `noexcept` spec.
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>) // required
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T>)
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  // TODO converting assignment (U&&, optional<U> const&, optional<U>&&): deferred until the
  // _can_convert* base traits land, for the same reason as the converting constructors above.

  using _base::emplace;

  // [optional.swap], swap
  constexpr void swap(optional &) noexcept(true); // TODO noexcept

  // [optional.observe], observers
  using _base::has_value;
  using _base::operator bool;
  using _base::operator*;
  using _base::operator->;
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
};

template <class T> optional(T) -> optional<T>;

template <class T> class optional<T &> : private detail::_optional_base<T &> {
  using _base = detail::_optional_base<T &>;

public:
  using value_type = T;

public:
  // [optional.ref.ctor], constructors
  constexpr optional() noexcept : _base(::std::nullopt) {}
  constexpr optional(::std::nullopt_t) noexcept : optional() {}
  constexpr optional(optional const &rhs) noexcept = default;

  template <class Arg>
  constexpr explicit optional(::std::in_place_t, Arg &&arg) noexcept //
      : _base(::std::in_place, FWD(arg))
  {
  }

  // TODO converting constructors (U&&, optional<U>&, optional<U> const&, optional<U>&&,
  // optional<U> const&&): deferred until the [optional.ref.ctor] constraint set lands. Left
  // undeclared on purpose — an unconstrained optional(U&&) would hijack the copy ctor.

  constexpr ~optional() = default;

  // [optional.ref.assign], assignment
  constexpr optional &operator=(::std::nullopt_t) noexcept
  {
    this->v_ = nullptr;
    return *this;
  }
  constexpr optional &operator=(optional const &rhs) noexcept = default;

  // TODO reference_constructs_from_temporary_v is a C++23 trait with no portable C++20
  // fallback; the dangling-reference guard from [optional.ref.assign]'s Constraints is
  // deferred (same gap as the in_place ctor above, which binds via addressof(arg) directly
  // rather than through _convert_ref_init_val's convert-then-bind).
  template <class U>
  constexpr T &emplace(U &&u) noexcept(::std::is_nothrow_constructible_v<T &, U>)
    requires ::std::is_constructible_v<T &, U>
  {
    _convert_ref_init_val(FWD(u));
    return *this->v_;
  }

  // [optional.ref.swap], swap
  constexpr void swap(optional &rhs) noexcept;

  // [optional.ref.observe], observers. Unlike optional<T>, const does not propagate to the
  // referent: *this being const only means the stored pointer can't be rebound, not that T
  // becomes T const -- so these all return plain T&/T*, and there is only ever one overload
  // (no ref-qualifier/const overload set like optional<T>'s).
  constexpr T *operator->() const noexcept
  {
    ASSERT(this->v_ != nullptr); // LCOV_EXCL_LINE
    return this->v_;
  }
  constexpr T &operator*() const noexcept { return *(this->operator->()); }
  constexpr explicit operator bool() const noexcept { return this->v_ != nullptr; }
  constexpr bool has_value() const noexcept { return this->v_ != nullptr; }
  constexpr T &value() const; // freestanding-deleted
  template <class U = ::std::remove_cv_t<T>> constexpr ::std::remove_cv_t<T> value_or(U &&u) const;

  // [optional.ref.monadic], monadic operations
  template <class F> constexpr auto and_then(F &&f) const;
  template <class F> constexpr optional<::std::invoke_result_t<F, T &>> transform(F &&f) const;
  template <class F> constexpr optional or_else(F &&f) const;

  // [optional.ref.mod], modifiers
  constexpr void reset() noexcept;

private:
  // [optional.ref.expos], exposition only helper functions. Binds a reference to `u` (through
  // whatever conversion T& requires) and stores its address -- used by emplace above and, once
  // implemented, the converting constructors/assignment.
  template <class U> constexpr void _convert_ref_init_val(U &&u) noexcept(::std::is_nothrow_constructible_v<T &, U>)
  {
    T &r(FWD(u));
    this->v_ = ::std::addressof(r);
  }
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
