// Copyright (c) 2025 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_EXPECTED
#define INCLUDE_PFN_EXPECTED

#include <cassert>
#include <concepts>
#include <cstring>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#ifdef FWD
#pragma push_macro("FWD")
#define INCLUDE_PFN_EXPECTED__POP_FWD
#undef FWD
#endif

// Also defined in fn/detail/fwd_macro.hpp but pfn/* headers are standalone
#define FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#ifdef ASSERT
#pragma push_macro("ASSERT")
#define INCLUDE_PFN_EXPECTED__POP_ASSERT
#undef ASSERT
#endif

// LIBFN_ASSERT is a customization point for the user
#ifdef LIBFN_ASSERT
#define ASSERT(...) LIBFN_ASSERT(__VA_ARGS__)
#else
#define ASSERT(...) assert((__VA_ARGS__) == true)
#endif

// This header is a polyfill for std::expected, tracking the C++ working draft
// and changes planned for the future standard revisions. As a deliberate extension,
// member functions whose `noexcept` specification is left unspecified by the
// standard are given a `noexcept` clause derived from the properties of the
// underlying types T, E and (where applicable) invocable arguments. Each such
// clause is marked inline with a "// extension" trailing comment.

namespace pfn {

template <class E> class bad_expected_access;

template <> class bad_expected_access<void> : public ::std::exception {
protected:
  bad_expected_access() noexcept = default;
  bad_expected_access(bad_expected_access const &) noexcept = default;
  bad_expected_access(bad_expected_access &&) noexcept = default;
  bad_expected_access &operator=(bad_expected_access const &) noexcept = default;
  bad_expected_access &operator=(bad_expected_access &&) noexcept = default;
  ~bad_expected_access() noexcept = default;

public:
  [[nodiscard]] char const *what() const noexcept override
  {
    static char const msg_[] = "bad access to expected without expected value";
    return msg_;
  }
};

template <class E> class bad_expected_access : public bad_expected_access<void> {
public:
  explicit bad_expected_access(E e) : e_(std::move(e)) {}
  [[nodiscard]] char const *what() const noexcept override { return bad_expected_access<void>::what(); };
  E &error() & noexcept { return e_; }
  E const &error() const & noexcept { return e_; }
  E &&error() && noexcept { return ::std::move(e_); }
  E const &&error() const && noexcept { return ::std::move(e_); }

private:
  E e_;
};

struct unexpect_t {
  explicit unexpect_t() = default;
};
constexpr inline unexpect_t unexpect{};

template <class E> class unexpected;

namespace detail {
template <typename> constexpr bool _is_some_unexpected = false;
template <typename T> constexpr bool _is_some_unexpected<::pfn::unexpected<T>> = true;

template <typename T>
constexpr bool _is_valid_unexpected = //
    ::std::is_object_v<T>             // i.e. not a reference or void or function
    && not ::std::is_array_v<T>       //
    && not _is_some_unexpected<T>     //
    && not ::std::is_const_v<T>       //
    && not ::std::is_volatile_v<T>;

// Helper used as noexcept(...) operand where we want to evaluate both:
// * noexcept of an expression itself (e.g. operator==) AND
// * noexcept of the expression's implicit conversion to bool
// May only be used in unevaluated contexts; any ODR-use will trigger a link error.
constexpr bool _implicit_to_bool(bool) noexcept;
} // namespace detail

template <class E> class unexpected {
  static_assert(detail::_is_valid_unexpected<E>);

public:
  constexpr unexpected(unexpected const &) = default;
  constexpr unexpected(unexpected &&) = default;

  template <class Err = E>
  constexpr explicit unexpected(Err &&e) noexcept(::std::is_nothrow_constructible_v<E, Err>)
    requires(not ::std::is_same_v<::std::remove_cvref_t<Err>, unexpected> &&        //
             not ::std::is_same_v<::std::remove_cvref_t<Err>, ::std::in_place_t> && //
             ::std::is_constructible_v<E, Err>)
      : e_(FWD(e))
  {
  }

  template <class... Args>
  constexpr explicit unexpected(::std::in_place_t, Args &&...a) noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  template <class U, class... Args>
  constexpr explicit unexpected(::std::in_place_t, ::std::initializer_list<U> i, Args &&...a) noexcept(
      ::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : e_(i, FWD(a)...)
  {
  }

  constexpr unexpected &operator=(unexpected const &) = default;
  constexpr unexpected &operator=(unexpected &&) = default;

  constexpr E const &error() const & noexcept { return e_; };
  constexpr E &error() & noexcept { return e_; };
  constexpr E const &&error() const && noexcept { return ::std::move(e_); };
  constexpr E &&error() && noexcept { return ::std::move(e_); };

  constexpr void swap(unexpected &other) noexcept(::std::is_nothrow_swappable_v<E>)
  {
    static_assert(::std::is_swappable_v<E>);
    using ::std::swap;
    swap(e_, other.e_);
  }

  template <class E2>
  constexpr friend bool operator==(unexpected const &x, unexpected<E2> const &y) //
      noexcept(noexcept(detail::_implicit_to_bool(x.error() == y.error())))      // extension
  {
    return x.error() == y.error();
  }

  constexpr friend void swap(unexpected &x, unexpected &y) noexcept(noexcept(x.swap(y)))
    requires ::std::is_swappable_v<E>
  {
    x.swap(y);
  }

private:
  E e_;
};

template <class E> unexpected(E) -> unexpected<E>;

namespace detail {
template <typename T, typename E>
constexpr bool _is_valid_expected =                                   //
    not ::std::is_reference_v<T>                                      //
    && not ::std::is_function_v<T>                                    //
    && not ::std::is_same_v<::std::remove_cv_t<T>, ::std::in_place_t> //
    && not ::std::is_same_v<::std::remove_cv_t<T>, unexpect_t>        //
    && not _is_some_unexpected<::std::remove_cv_t<T>>                 //
    && detail::_is_valid_unexpected<E>;

// Internal union wrapper for the value/error storage. Copy/move ctors
// are defaulted iff both `T` and `E` are trivially copy/move-constructible;
// otherwise deleted (callers placement-new the appropriate arm).
// The in_place / unexpect ctors activate `v_` / `e_` respectively.
template <class T, class E> union _storage_union_t {
  using _value_t = T;
  T v_;
  E e_;

  constexpr _storage_union_t(_storage_union_t const &) = delete;
  constexpr _storage_union_t(_storage_union_t const &) //
    requires(::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<E>)
  = default;
  constexpr _storage_union_t(_storage_union_t &&) = delete;
  constexpr _storage_union_t(_storage_union_t &&) //
    requires(::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<E>)
  = default;
  constexpr _storage_union_t &operator=(_storage_union_t const &) = delete;
  constexpr _storage_union_t &operator=(_storage_union_t &&) = delete;

  constexpr explicit _storage_union_t() noexcept {}

  template <class... Args>
  constexpr explicit _storage_union_t(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>)
    requires ::std::is_constructible_v<T, Args...>
      : v_(FWD(a)...)
  {
  }
  template <class... Args>
  constexpr explicit _storage_union_t(unexpect_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  constexpr ~_storage_union_t() noexcept
    requires(::std::is_trivially_destructible_v<T> && ::std::is_trivially_destructible_v<E>)
  = default;
  constexpr ~_storage_union_t() noexcept {}

  // Implements the reinit-expected helper from [expected.object.assign].
  template <typename New, typename Old, typename... Args>
  static constexpr void _reinit(New *newp, Old *oldp, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<New, Args...>)
  {
    if constexpr (::std::is_nothrow_constructible_v<New, Args...>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp, ::std::forward<Args>(args)...);
    } else if constexpr (::std::is_nothrow_move_constructible_v<New>) {
      New tmp(::std::forward<Args>(args)...);
      ::std::destroy_at(oldp);
      ::std::construct_at(newp, std::move(tmp));
    } else if constexpr (::std::is_trivially_copyable_v<Old>) {
      // Workaround for https://github.com/llvm/llvm-project/issues/196520:
      // clang on aarch64 sinks the snapshot load past the store-through-newp
      // when Old's TBAA tag differs from New's, corrupting the strong-EG
      // restoration on catch. A byte-buffer snapshot via std::memcpy is opaque
      // to TBAA, and preserving *oldp in place across the try (no destroy_at)
      // makes the catch a plain byte restore -- which for trivially-copyable
      // (and therefore trivially-destructible per [class.prop]/1). Old is
      // observationally identical to the destroy-and-recreate branch below.
      if (not ::std::is_constant_evaluated()) {
        alignas(Old) unsigned char _bytes[sizeof(Old)];
        ::std::memcpy(_bytes, oldp, sizeof(Old));
        try {
          ::std::construct_at(newp, ::std::forward<Args>(args)...);
        } catch (...) {
          ::std::memcpy(oldp, _bytes, sizeof(Old));
          throw;
        }
      } else {
        // LCOV_EXCL_START constant-evaluated only; runtime branches are above and below
        Old tmp(std::move(*oldp));
        ::std::destroy_at(oldp);
        try {
          ::std::construct_at(newp, ::std::forward<Args>(args)...);
        } catch (...) {
          ::std::construct_at(oldp, std::move(tmp));
          throw;
        }
        // LCOV_EXCL_STOP
      }
    } else {
      Old tmp(std::move(*oldp));
      ::std::destroy_at(oldp);
      try {
        ::std::construct_at(newp, ::std::forward<Args>(args)...);
      } catch (...) {
        ::std::construct_at(oldp, std::move(tmp));
        throw;
      }
    }
  }
};

// Void specialization: value state activates a trivial `_dummy_t v_`
// placeholder so the union always has an active member (required by
// constant evaluation). All `v_` operations are no-ops at runtime.
template <class E> union _storage_union_t<void, E> {
  struct _dummy_t final {
    constexpr _dummy_t() noexcept = default;
  };

  using _value_t = _dummy_t;
  _dummy_t v_;
  E e_;

  constexpr _storage_union_t(_storage_union_t const &) = delete;
  constexpr _storage_union_t(_storage_union_t const &) //
    requires(::std::is_trivially_copy_constructible_v<E>)
  = default;
  constexpr _storage_union_t(_storage_union_t &&) = delete;
  constexpr _storage_union_t(_storage_union_t &&) //
    requires(::std::is_trivially_move_constructible_v<E>)
  = default;
  constexpr _storage_union_t &operator=(_storage_union_t const &) = delete;
  constexpr _storage_union_t &operator=(_storage_union_t &&) = delete;

  constexpr explicit _storage_union_t() noexcept {}
  constexpr explicit _storage_union_t(::std::in_place_t) noexcept : v_{} {}

  template <class... Args>
  constexpr explicit _storage_union_t(unexpect_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  constexpr ~_storage_union_t() noexcept
    requires(::std::is_trivially_destructible_v<E>)
  = default;
  constexpr ~_storage_union_t() noexcept {}

  // [expected.void.assign] mandates direct construction (no temporary).
  template <typename New, typename Old, typename... Args>
  static constexpr void _reinit(New *newp, Old *oldp, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<New, Args...>)
  {
    if constexpr (::std::is_same_v<New, _dummy_t>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp);
    } else if constexpr (::std::is_nothrow_constructible_v<New, Args...>) {
      ::std::destroy_at(oldp);
      ::std::construct_at(newp, ::std::forward<Args>(args)...);
    } else {
      // On exception the trivial `_dummy_t` *oldp is reconstructed so the
      // union always has an active member (required for constant evaluation).
      ::std::destroy_at(oldp);
      try {
        ::std::construct_at(newp, ::std::forward<Args>(args)...);
      } catch (...) {
        ::std::construct_at(oldp);
        throw;
      }
    }
  }
};

// Shared storage base for ::pfn::expected. Members are public because the
// inheritance is private; sibling-instantiation access goes through the
// `template <class, class> friend class expected;` declaration on the derived.
template <class T, class E> struct _storage {
  using _storage_t = _storage_union_t<T, E>;
  // `T` for non-void, trivial `_dummy_t` for void; used as observer return
  // type (void value-returning overloads are requires-disabled, so the
  // placeholder is never observable) and in destructor constraints.
  using _value_t = _storage_t::_value_t;
  _storage_t storage_;
  bool set_;

  template <class... Args>
  constexpr explicit _storage(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, Args...>
      : storage_(::std::in_place, FWD(a)...), set_(true)
  {
  }
  template <class... Args>
  constexpr explicit _storage(unexpect_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : storage_(unexpect, FWD(a)...), set_(false)
  {
  }

  constexpr explicit _storage(bool s, auto &&src) : storage_(), set_(s)
  {
    if (set_) {
      if constexpr (::std::is_void_v<T>)
        ::std::construct_at(::std::addressof(storage_.v_)); // LCOV_EXCL_LINE trivially constructible
      else
        ::std::construct_at(::std::addressof(storage_.v_), FWD(src).v_);
    } else
      ::std::construct_at(::std::addressof(storage_.e_), FWD(src).e_);
  }

  constexpr _storage(_storage const &) = default;
  constexpr _storage(_storage &&) = default;
  constexpr _storage &operator=(_storage const &) = delete;
  constexpr _storage &operator=(_storage &&) = delete;

  constexpr ~_storage() noexcept
    requires(::std::is_trivially_destructible_v<_value_t> && ::std::is_trivially_destructible_v<E>)
  = default;
  constexpr ~_storage() //
    requires(::std::is_trivially_destructible_v<_value_t> && not ::std::is_trivially_destructible_v<E>)
  {
    if (not set_)
      ::std::destroy_at(::std::addressof(storage_.e_));
  }
  constexpr ~_storage() //
    requires(not ::std::is_trivially_destructible_v<_value_t> && ::std::is_trivially_destructible_v<E>)
  {
    if (set_)
      ::std::destroy_at(::std::addressof(storage_.v_));
  }
  constexpr ~_storage() //
    requires(not ::std::is_trivially_destructible_v<_value_t> && not ::std::is_trivially_destructible_v<E>)
  {
    if (set_)
      ::std::destroy_at(::std::addressof(storage_.v_));
    else
      ::std::destroy_at(::std::addressof(storage_.e_));
  }

  constexpr _value_t const *operator->() const noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(set_);
    return ::std::addressof(storage_.v_);
  }
  constexpr _value_t *operator->() noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(set_);
    return ::std::addressof(storage_.v_);
  }
  constexpr _value_t const &operator*() const & noexcept
    requires(not ::std::is_void_v<T>)
  {
    return *(this->operator->());
  }
  constexpr _value_t &operator*() & noexcept
    requires(not ::std::is_void_v<T>)
  {
    return *(this->operator->());
  }
  constexpr _value_t const &&operator*() const && noexcept
    requires(not ::std::is_void_v<T>)
  {
    return ::std::move(*(this->operator->()));
  }
  constexpr _value_t &&operator*() && noexcept
    requires(not ::std::is_void_v<T>)
  {
    return ::std::move(*(this->operator->()));
  }
  constexpr void operator*() const & noexcept
    requires(::std::is_void_v<T>)
  {
    ASSERT(set_);
  }
  constexpr void operator*() & noexcept
    requires(::std::is_void_v<T>)
  {
    ASSERT(set_);
  }
  constexpr void operator*() const && noexcept
    requires(::std::is_void_v<T>)
  {
    ASSERT(set_);
  }
  constexpr void operator*() && noexcept
    requires(::std::is_void_v<T>)
  {
    ASSERT(set_);
  }
  constexpr explicit operator bool() const noexcept { return set_; }
  constexpr bool has_value() const noexcept { return set_; }
  constexpr bool has_error() const noexcept { return !set_; } // P3798
  constexpr _value_t const &value() const &
    requires(not ::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(storage_.e_);
    return storage_.v_;
  }
  constexpr _value_t &value() &
    requires(not ::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(::std::as_const(storage_.e_));
    return storage_.v_;
  }
  constexpr _value_t const &&value() const &&
    requires(not ::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_constructible_v<E, E const &&>);
    if (not set_)
      throw bad_expected_access<E>(::std::move(storage_.e_));
    return ::std::move(storage_.v_);
  }
  constexpr _value_t &&value() &&
    requires(not ::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_constructible_v<E, E &&>);
    if (not set_)
      throw bad_expected_access<E>(::std::move(storage_.e_));
    return ::std::move(storage_.v_);
  }
  constexpr void value() const &
    requires(::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(storage_.e_);
  }
  constexpr void value() &&
    requires(::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_move_constructible_v<E>);
    if (not set_)
      throw bad_expected_access<E>(::std::move(storage_.e_));
  }
  constexpr E const &error() const & noexcept
  {
    ASSERT(not set_);
    return storage_.e_;
  }
  constexpr E &error() & noexcept
  {
    ASSERT(not set_);
    return storage_.e_;
  }
  constexpr E const &&error() const && noexcept
  {
    ASSERT(not set_);
    return ::std::move(storage_.e_);
  }
  constexpr E &&error() && noexcept
  {
    ASSERT(not set_);
    return ::std::move(storage_.e_);
  }

  template <class U>
  constexpr T value_or(U &&v) const &                                                              //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_convertible_v<U, T>) // extension
    requires(not ::std::is_void_v<T>)
  {
    static_assert(::std::is_copy_constructible_v<T>);
    static_assert(::std::is_convertible_v<U, T>);
    return set_ ? storage_.v_ : static_cast<T>(FWD(v));
  }
  template <class U>
  constexpr T value_or(U &&v) &&                                                                   //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_convertible_v<U, T>) // extension
    requires(not ::std::is_void_v<T>)
  {
    static_assert(::std::is_move_constructible_v<T>);
    static_assert(::std::is_convertible_v<U, T>);
    return set_ ? ::std::move(storage_.v_) : static_cast<T>(FWD(v));
  }

  template <class G = E>
  constexpr E error_or(G &&e) const &                                                              //
      noexcept(::std::is_nothrow_copy_constructible_v<E> && ::std::is_nothrow_convertible_v<G, E>) // extension
  {
    static_assert(::std::is_copy_constructible_v<E>);
    static_assert(::std::is_convertible_v<G, E>);
    if (set_) {
      return FWD(e);
    }
    return storage_.e_;
  }
  template <class G = E>
  constexpr E error_or(G &&e) &&                                                                   //
      noexcept(::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_convertible_v<G, E>) // extension
  {
    static_assert(::std::is_move_constructible_v<E>);
    static_assert(::std::is_convertible_v<G, E>);
    if (set_) {
      return FWD(e);
    }
    return ::std::move(storage_.e_);
  }

  // Unchecked value accessor used by monadic helpers; `value()` throws,
  // this asserts the precondition.
  constexpr _value_t const &_value() const & noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(set_);
    return storage_.v_;
  }
  constexpr _value_t &_value() & noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(set_);
    return storage_.v_;
  }
  constexpr _value_t const &&_value() const && noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(set_);
    return ::std::move(storage_.v_);
  }
  constexpr _value_t &&_value() && noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(set_);
    return ::std::move(storage_.v_);
  }

  // Assignment body shared by the public expected operator= overloads,
  // which keep their constraints/noexcept clauses and forward `s` here as
  // lvalue or rvalue. For T=void all `storage_.v_` operations are no-ops
  // at runtime (trivial `_dummy_t`) but still track the active union member.
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
  template <class U> constexpr void _assign_value(U &&s)
  {
    if (set_) {
      storage_.v_ = FWD(s);
    } else {
      _storage_t::_reinit(::std::addressof(storage_.v_), ::std::addressof(storage_.e_), FWD(s));
      set_ = true;
    }
  }
  constexpr void _assign_unexpected(auto &&s)
  {
    if (not set_) {
      storage_.e_ = FWD(s).error();
    } else {
      _storage_t::_reinit(::std::addressof(storage_.e_), ::std::addressof(storage_.v_), FWD(s).error());
      set_ = false;
    }
  }

  // Cross-state swap; lhs holds value, rhs holds error.
  static constexpr void _swap_helper(_storage &lhs, _storage &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_constructible_v<E>)
  {
    if constexpr (::std::is_nothrow_move_constructible_v<E>) {
      E tmp(::std::move(rhs.storage_.e_));
      ::std::destroy_at(::std::addressof(rhs.storage_.e_));
      try {
        ::std::construct_at(::std::addressof(rhs.storage_.v_), ::std::move(lhs.storage_.v_));
        ::std::destroy_at(::std::addressof(lhs.storage_.v_));
        ::std::construct_at(::std::addressof(lhs.storage_.e_), ::std::move(tmp));
      } catch (...) {
        ::std::construct_at(::std::addressof(rhs.storage_.e_), ::std::move(tmp));
        throw;
      }
    } else {
      auto tmp(::std::move(lhs.storage_.v_));
      ::std::destroy_at(::std::addressof(lhs.storage_.v_));
      try {
        ::std::construct_at(::std::addressof(lhs.storage_.e_), ::std::move(rhs.storage_.e_));
        ::std::destroy_at(::std::addressof(rhs.storage_.e_));
        ::std::construct_at(::std::addressof(rhs.storage_.v_), ::std::move(tmp));
      } catch (...) {
        ::std::construct_at(::std::addressof(lhs.storage_.v_), ::std::move(tmp));
        throw;
      }
    }
    lhs.set_ = false;
    rhs.set_ = true;
  }

  constexpr void _swap_with(_storage &rhs)
  {
    if (set_ == rhs.set_) {
      if (set_) {
        if constexpr (not ::std::is_void_v<T>) {
          using ::std::swap;
          swap(storage_.v_, rhs.storage_.v_);
        }
      } else {
        using ::std::swap;
        swap(storage_.e_, rhs.storage_.e_);
      }
    } else if constexpr (::std::is_void_v<T>) {
      // [expected.void.swap] mandates a single E move on cross-state.
      // On exception from E's move, restore the `_dummy_t` value so the union
      // always has an active member (required for constant evaluation).
      if (set_) {
        ::std::destroy_at(::std::addressof(storage_.v_));
        try {
          ::std::construct_at(::std::addressof(storage_.e_), ::std::move(rhs.storage_.e_));
        } catch (...) {
          ::std::construct_at(::std::addressof(storage_.v_));
          throw;
        }
        ::std::destroy_at(::std::addressof(rhs.storage_.e_));
        ::std::construct_at(::std::addressof(rhs.storage_.v_));
        set_ = false;
        rhs.set_ = true;
      } else {
        rhs._swap_with(*this);
      }
    } else if (set_) {
      _swap_helper(*this, rhs);
    } else {
      _swap_helper(rhs, *this);
    }
  }

  template <class... Args>
  constexpr _value_t &emplace(Args &&...args) noexcept
    requires(not ::std::is_void_v<T> && ::std::is_nothrow_constructible_v<T, Args...>)
  {
    if (set_) {
      ::std::destroy_at(::std::addressof(storage_.v_));
    } else {
      ::std::destroy_at(::std::addressof(storage_.e_));
      set_ = true;
    }
    return *::std::construct_at(::std::addressof(storage_.v_), std::forward<Args>(args)...);
  }

  template <class U, class... Args>
  constexpr _value_t &emplace(::std::initializer_list<U> il, Args &&...args) noexcept
    requires(not ::std::is_void_v<T> && ::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)
  {
    if (set_) {
      ::std::destroy_at(::std::addressof(storage_.v_));
    } else {
      ::std::destroy_at(::std::addressof(storage_.e_));
      set_ = true;
    }
    return *::std::construct_at(::std::addressof(storage_.v_), il, std::forward<Args>(args)...);
  }

  constexpr void emplace() noexcept
    requires(::std::is_void_v<T>)
  {
    if (not set_) {
      ::std::destroy_at(::std::addressof(storage_.e_));
      ::std::construct_at(::std::addressof(storage_.v_));
      set_ = true;
    }
  }
};

} // namespace detail

template <class T, class E> class expected;
template <class E> class expected<void, E>;

namespace detail {
template <typename> constexpr bool _is_some_expected = false;
template <typename T, typename E> constexpr bool _is_some_expected<::pfn::expected<T, E>> = true;
} // namespace detail

template <class T, class E> class expected : private detail::_storage<T, E> {
  static_assert(detail::_is_valid_expected<T, E>);
  using _base = detail::_storage<T, E>;

  template <class U, class G, class UF, class GF>
  using _can_convert_detail = ::std::bool_constant<                           //
      not(::std::is_same_v<T, U> && ::std::is_same_v<E, G>)                   //
      && ::std::is_constructible_v<T, UF>                                     //
      && ::std::is_constructible_v<E, GF>                                     //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> &>       //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G>>         //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const &> //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const>   //
      && (::std::is_same_v<bool, ::std::remove_cv_t<T>>                       //
          || (                                                                //
              not ::std::is_constructible_v<T, expected<U, G> &>              //
              && not ::std::is_constructible_v<T, expected<U, G>>             //
              && not ::std::is_constructible_v<T, expected<U, G> const &>     //
              && not ::std::is_constructible_v<T, expected<U, G> const>       //
              && not ::std::is_convertible_v<expected<U, G> &, T>             //
              && not ::std::is_convertible_v<expected<U, G> &&, T>            //
              && not ::std::is_convertible_v<expected<U, G> const &, T>       //
              && not ::std::is_convertible_v<expected<U, G> const &&, T>))>;

  template <class U, class G> using _can_copy_convert = _can_convert_detail<U, G, U const &, G const &>;
  template <class U, class G> using _can_move_convert = _can_convert_detail<U, G, U, G>;
  template <class U, class G> friend class expected;

  template <class U>
  using _can_convert = ::std::bool_constant<                            //
      not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t> //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, unexpect_t>     // LWG4222
      && not ::std::is_same_v<expected, ::std::remove_cvref_t<U>>       //
      && not detail::_is_some_unexpected<::std::remove_cvref_t<U>>      //
      && ::std::is_constructible_v<T, U>                                //
      && (not ::std::is_same_v<bool, ::std::remove_cv_t<T>>             //
          || not detail::_is_some_expected<::std::remove_cvref_t<U>>)>;

  template <class U>
  using _can_convert_assign = ::std::bool_constant<                //
      not ::std::is_same_v<expected, ::std::remove_cvref_t<U>>     //
      && not detail::_is_some_unexpected<::std::remove_cvref_t<U>> //
      && ::std::is_constructible_v<T, U>                           //
      && ::std::is_assignable_v<T &, U>                            //
      && (::std::is_nothrow_constructible_v<T, U>                  //
          || ::std::is_nothrow_move_constructible_v<T>             //
          || ::std::is_nothrow_move_constructible_v<E>)>;

  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self)._value())>
               && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self)._value())>
             && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::error_type, typename ::std::remove_cvref_t<Self>::error_type>);
    if (self.has_value()) {
      return ::std::invoke(FWD(fn), FWD(self)._value());
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>
               && ::std::is_nothrow_constructible_v<T, decltype(FWD(self)._value())>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>
             && ::std::is_constructible_v<T, decltype(FWD(self)._value())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::value_type, typename ::std::remove_cvref_t<Self>::value_type>);
    if (self.has_value()) {
      return result_t(::std::in_place, FWD(self)._value());
    }
    return ::std::invoke(FWD(fn), FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self)._value())>
               && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>
               && (::std::is_void_v<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>
                   || ::std::is_nothrow_constructible_v<
                       ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>,
                       ::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>))
    requires(::std::is_invocable_v<Fn, decltype(FWD(self)._value())>
             && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>;
    static_assert(detail::_is_valid_expected<value_t, E>);
    using result_t = expected<value_t, E>;
    if (self.has_value()) {
      if constexpr (not ::std::is_void_v<value_t>) {
        static_assert(::std::is_constructible_v<value_t, ::std::invoke_result_t<Fn, decltype(FWD(self)._value())>>);
        return result_t(::std::in_place, ::std::invoke(FWD(fn), FWD(self)._value()));
      } else {
        ::std::invoke(FWD(fn), FWD(self)._value());
        return result_t(::std::in_place);
      }
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>
               && ::std::is_nothrow_constructible_v<T, decltype(FWD(self)._value())>
               && ::std::is_nothrow_constructible_v<
                   ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>,
                   ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>
             && ::std::is_constructible_v<T, decltype(FWD(self)._value())>)
  {
    using error_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_valid_unexpected<error_t>);
    static_assert(::std::is_constructible_v<error_t, ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>);
    using result_t = expected<T, error_t>;
    if (not self.has_value()) {
      return result_t(unexpect, ::std::invoke(FWD(fn), FWD(self).error()));
    }
    return result_t(::std::in_place, FWD(self)._value());
  }

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.object.cons], constructors
  constexpr expected()                                       //
      noexcept(::std::is_nothrow_default_constructible_v<T>) // extension
    requires ::std::is_default_constructible_v<T>
      : _base(::std::in_place)
  {
  } // LCOV_EXCL_LINE

  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &s)                                                                //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<E>
             && ::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<E>)
  = default;
  constexpr expected(expected const &s)                                                                //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<E>
             && (not ::std::is_trivially_copy_constructible_v<T> || not ::std::is_trivially_copy_constructible_v<E>))
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr expected(expected &&s)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<E>
             && ::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<E>)
  = default;
  constexpr expected(expected &&s)                                                                     //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<E>
             && (not ::std::is_trivially_move_constructible_v<T> || not ::std::is_trivially_move_constructible_v<E>))
      : _base(s.set_, FWD(s).storage_)
  {
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U const &, T> || not ::std::is_convertible_v<G const &, E>)
      expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &>
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(_can_copy_convert<U, G>::value)
      : _base(s.set_, FWD(s).storage_)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, E>)
      expected(expected<U, G> &&s)                                                                 //
      noexcept(::std::is_nothrow_constructible_v<T, U> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(_can_move_convert<U, G>::value)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) expected(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)                 // extension
    requires(_can_convert<U>::value)
      : _base(::std::in_place, FWD(v))
  {
  }

  template <class G>
  constexpr explicit(!::std::is_convertible_v<G const &, E>) expected(unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                               // extension
    requires(::std::is_constructible_v<E, G const &>)
      : _base(unexpect, ::std::forward<G const &>(g.error()))
  {
  }

  template <class G>
  constexpr explicit(!::std::is_convertible_v<G, E>) expected(unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                          // extension
    requires(::std::is_constructible_v<E, G>)
      : _base(unexpect, ::std::forward<G>(g.error()))
  {
  }

  template <class... Args>
  constexpr explicit expected(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>) // extension
    requires ::std::is_constructible_v<T, Args...>
      : _base(::std::in_place, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::std::in_place_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)  // extension
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
      : _base(::std::in_place, il, FWD(a)...)
  {
  }
  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...a)        //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>) // extension
    requires ::std::is_constructible_v<E, Args...>
      : _base(unexpect, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(unexpect_t, ::std::initializer_list<U> il, Args &&...a)       //
      noexcept(::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>) // extension
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : _base(unexpect, il, FWD(a)...)
  {
  }

  // [expected.object.dtor], destructor inherited from _storage

  // [expected.object.assign], assignment; bodies delegate to _storage helpers
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &s) //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>
               && ::std::is_nothrow_copy_assignable_v<E> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_assignable_v<T> && ::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<E>
             && ::std::is_copy_constructible_v<E>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>))
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }

  constexpr expected &operator=(expected &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>
               && ::std::is_nothrow_move_assignable_v<E> && ::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T> && ::std::is_move_constructible_v<E>
             && ::std::is_move_assignable_v<E>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>))
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  template <class U = T>
  constexpr expected &operator=(U &&s)                                                            //
      noexcept(::std::is_nothrow_assignable_v<T &, U> && ::std::is_nothrow_constructible_v<T, U>) // extension
    requires(_can_convert_assign<U>::value)
  {
    this->_assign_value(FWD(s));
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<E &, G const &>
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(::std::is_constructible_v<E, G const &> && ::std::is_assignable_v<E &, G const &>
             && (::std::is_nothrow_constructible_v<E, G const &> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<E>))
  {
    this->_assign_unexpected(s);
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> &&s)                                                //
      noexcept(::std::is_nothrow_assignable_v<E &, G> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(::std::is_constructible_v<E, G> && ::std::is_assignable_v<E &, G>
             && (::std::is_nothrow_constructible_v<E, G> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<E>))
  {
    this->_assign_unexpected(::std::move(s));
    return *this;
  }

  // [expected.object.emplace], emplace inherited from _storage
  using _base::emplace;

  // [expected.object.swap], swap; body delegates to _storage helper
  constexpr void
  swap(expected &rhs) noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_swappable_v<T>
                               && ::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_swappable_v<E>)
    requires(::std::is_swappable_v<T> && ::std::is_swappable_v<E> && ::std::is_move_constructible_v<T>
             && ::std::is_move_constructible_v<E>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<E>))
  {
    this->_swap_with(rhs);
  }

  constexpr friend void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }

  // [expected.object.obs], observers inherited from _storage
  using _base::operator*;
  using _base::operator->;
  using _base::operator bool;
  using _base::error;
  using _base::error_or;
  using _base::has_error;
  using _base::has_value;
  using _base::value;
  using _base::value_or;

  // [expected.object.monadic], monadic operations
  template <class F>
  constexpr auto and_then(F &&f) &                 //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                             //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &           //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                       //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                 //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                             //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &           //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                       //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                 //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                             //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &           //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                       //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform_error(F &&f) &                 //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                             //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &           //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                       //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }

  // [expected.object.eq], equality operators
  template <class T2, class E2>
    requires(not ::std::is_void_v<T2>)
  constexpr friend bool operator==(expected const &x, expected<T2, E2> const &y) //
      noexcept(noexcept(detail::_implicit_to_bool(*x == *y)) && noexcept(detail::_implicit_to_bool(x.error() == y.error())))           // extension
    requires ( //
requires {
      { *x == *y } -> std::convertible_to<bool>;
    } &&
requires {
    { x.error() == y.error() } -> std::convertible_to<bool>;
    })
  {
    if (x.has_value() != y.has_value()) {
      return false;
    }
    if (x.has_value()) {
      return *x == *y;
    }
    return x.error() == y.error();
  }
  template <class T2>
    requires(not detail::_is_some_expected<T2>)
  constexpr friend bool operator==(expected const &x, const T2 &v) //
      noexcept(noexcept(detail::_implicit_to_bool(*x == v)))       // extension
    requires requires {
      { *x == v } -> std::convertible_to<bool>;
    }
  {
    if (!x.has_value()) {
      return false;
    }
    return *x == v;
  }
  template <class E2>
  constexpr friend bool operator==(expected const &x, unexpected<E2> const &e) //
      noexcept(noexcept(detail::_implicit_to_bool(x.error() == e.error())))    // extension
    requires requires {
      { x.error() == e.error() } -> std::convertible_to<bool>;
    }
  {
    if (x.has_value()) {
      return false;
    }
    return x.error() == e.error();
  }
};

template <class E> class expected<void, E> : private detail::_storage<void, E> {
  static_assert(detail::_is_valid_unexpected<E>);
  using _base = detail::_storage<void, E>;

  template <class U, class G, class GF>
  using _can_convert_detail = ::std::bool_constant<                           //
      ::std::is_void_v<U> && ::std::is_constructible_v<E, GF>                 //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> &>       //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G>>         //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const &> //
      && not ::std::is_constructible_v<unexpected<E>, expected<U, G> const>>;

  template <class U, class G> using _can_copy_convert = _can_convert_detail<U, G, G const &>;
  template <class U, class G> using _can_move_convert = _can_convert_detail<U, G, G>;
  template <class U, class G> friend class expected;

  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn> && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>)
    requires(::std::is_invocable_v<Fn> && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::error_type, typename ::std::remove_cvref_t<Self>::error_type>);
    if (self.has_value()) {
      return ::std::invoke(FWD(fn));
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_some_expected<result_t>);
    static_assert(::std::is_same_v<typename result_t::value_type, typename ::std::remove_cvref_t<Self>::value_type>);
    if (self.has_value()) {
      return result_t(::std::in_place);
    }
    return ::std::invoke(FWD(fn), FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn> && ::std::is_nothrow_constructible_v<E, decltype(FWD(self).error())>
               && (::std::is_void_v<::std::invoke_result_t<Fn>>
                   || ::std::is_nothrow_constructible_v<::std::remove_cv_t<::std::invoke_result_t<Fn>>,
                                                        ::std::invoke_result_t<Fn>>))
    requires(::std::is_invocable_v<Fn> && ::std::is_constructible_v<E, decltype(FWD(self).error())>)
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn>>;
    static_assert(detail::_is_valid_expected<value_t, E>);
    using result_t = expected<value_t, E>;
    if (self.has_value()) {
      if constexpr (not ::std::is_void_v<value_t>) {
        static_assert(::std::is_constructible_v<value_t, ::std::invoke_result_t<Fn>>);
        return result_t(::std::in_place, ::std::invoke(FWD(fn)));
      } else {
        ::std::invoke(FWD(fn));
        return result_t(::std::in_place);
      }
    }
    return result_t(unexpect, FWD(self).error());
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(FWD(self).error())>
               && ::std::is_nothrow_constructible_v<
                   ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>,
                   ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>)
    requires(::std::is_invocable_v<Fn, decltype(FWD(self).error())>)
  {
    using error_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(FWD(self).error())>>;
    static_assert(detail::_is_valid_unexpected<error_t>);
    static_assert(::std::is_constructible_v<error_t, ::std::invoke_result_t<Fn, decltype(FWD(self).error())>>);
    using result_t = expected<void, error_t>;
    if (not self.has_value()) {
      return result_t(unexpect, ::std::invoke(FWD(fn), FWD(self).error()));
    }
    return result_t(::std::in_place);
  }

public:
  using value_type = void;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.void.cons], constructors
  constexpr expected() noexcept : _base(::std::in_place) {}
  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &)                                                         //
    requires(::std::is_copy_constructible_v<E> && ::std::is_trivially_copy_constructible_v<E>) //
  = default;
  constexpr expected(expected const &s)                                                            //
      noexcept(::std::is_nothrow_copy_constructible_v<E>)                                          // extension
    requires(::std::is_copy_constructible_v<E> && not ::std::is_trivially_copy_constructible_v<E>) //
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr expected(expected &&s)                                                             //
    requires(::std::is_move_constructible_v<E> && ::std::is_trivially_move_constructible_v<E>) //
  = default;
  constexpr expected(expected &&s)                        //
      noexcept(::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<E> && not ::std::is_trivially_move_constructible_v<E>)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, E>) expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                                   // extension
    requires(_can_copy_convert<U, G>::value)
      : _base(s.set_, FWD(s).storage_)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G, E>) expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                              // extension
    requires(_can_move_convert<U, G>::value)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  template <class G>
  constexpr explicit(!::std::is_convertible_v<G const &, E>) expected(unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                               // extension
    requires(::std::is_constructible_v<E, G const &>)
      : _base(unexpect, ::std::forward<G const &>(g.error()))
  {
  }
  template <class G>
  constexpr explicit(!::std::is_convertible_v<G, E>) expected(unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                          // extension
    requires(::std::is_constructible_v<E, G>)
      : _base(unexpect, ::std::forward<G>(g.error()))
  {
  }

  constexpr explicit expected(::std::in_place_t) noexcept : _base(::std::in_place) {}

  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...a)        //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>) // extension
    requires ::std::is_constructible_v<E, Args...>
      : _base(unexpect, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(unexpect_t, ::std::initializer_list<U> il, Args &&...a)       //
      noexcept(::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>) // extension
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : _base(unexpect, il, FWD(a)...)
  {
  }

  // [expected.void.dtor], destructor inherited from _storage

  // [expected.void.assign], assignment; bodies delegate to _storage helpers
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &s)                                                  //
      noexcept(::std::is_nothrow_copy_assignable_v<E> && ::std::is_nothrow_copy_constructible_v<E>) // extension
    requires(::std::is_copy_assignable_v<E> && ::std::is_copy_constructible_v<E>)
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }

  constexpr expected &operator=(expected &&s)                                                       //
      noexcept(::std::is_nothrow_move_assignable_v<E> && ::std::is_nothrow_move_constructible_v<E>) // required
    requires(::std::is_move_constructible_v<E> && ::std::is_move_assignable_v<E>)
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<E &, G const &>
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(::std::is_constructible_v<E, G const &> && ::std::is_assignable_v<E &, G const &>)
  {
    this->_assign_unexpected(s);
    return *this;
  }

  template <class G>
  constexpr expected &operator=(unexpected<G> &&s)                                                //
      noexcept(::std::is_nothrow_assignable_v<E &, G> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(::std::is_constructible_v<E, G> && ::std::is_assignable_v<E &, G>)
  {
    this->_assign_unexpected(::std::move(s));
    return *this;
  }

  // [expected.void.emplace], emplace inherited from _storage
  using _base::emplace;

  // [expected.void.swap], swap; body delegates to _storage helper
  constexpr void swap(expected &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_swappable_v<E>)
    requires(::std::is_swappable_v<E> && ::std::is_move_constructible_v<E>)
  {
    this->_swap_with(rhs);
  }

  constexpr friend void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }

  // [expected.void.obs], observers inherited from _storage
  using _base::operator*;
  using _base::operator bool;
  using _base::error;
  using _base::error_or;
  using _base::has_error;
  using _base::has_value;
  using _base::value;

  // [expected.void.monadic], monadic operations
  template <class F>
  constexpr auto and_then(F &&f) &                 //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                             //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &           //
      noexcept(noexcept(_and_then(*this, FWD(f)))) // extension
      -> decltype(_and_then(*this, FWD(f)))
  {
    return _and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                       //
      noexcept(noexcept(_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_and_then(::std::move(*this), FWD(f)))
  {
    return _and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                 //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                             //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &           //
      noexcept(noexcept(_or_else(*this, FWD(f)))) // extension
      -> decltype(_or_else(*this, FWD(f)))
  {
    return _or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                       //
      noexcept(noexcept(_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_or_else(::std::move(*this), FWD(f)))
  {
    return _or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                 //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                             //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &           //
      noexcept(noexcept(_transform(*this, FWD(f)))) // extension
      -> decltype(_transform(*this, FWD(f)))
  {
    return _transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                       //
      noexcept(noexcept(_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform(::std::move(*this), FWD(f)))
  {
    return _transform(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform_error(F &&f) &                 //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                             //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &           //
      noexcept(noexcept(_transform_error(*this, FWD(f)))) // extension
      -> decltype(_transform_error(*this, FWD(f)))
  {
    return _transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                       //
      noexcept(noexcept(_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_transform_error(::std::move(*this), FWD(f)))
  {
    return _transform_error(::std::move(*this), FWD(f));
  }

  // [expected.void.eq], equality operators
  template <class T2, class E2>
    requires(::std::is_void_v<T2>)
  constexpr friend bool operator==(expected const &x, expected<T2, E2> const &y) //
      noexcept(noexcept(detail::_implicit_to_bool((x.error() == y.error()))))    // extension
    requires requires {
      { x.error() == y.error() } -> std::convertible_to<bool>;
    }
  {
    if (x.has_value() != y.has_value()) {
      return false;
    }
    if (x.has_value()) {
      return true;
    }
    return x.error() == y.error();
  }
  template <class E2>
  constexpr friend bool operator==(expected const &x, unexpected<E2> const &e) //
      noexcept(noexcept(detail::_implicit_to_bool(x.error() == e.error())))    // extension
    requires requires {
      { x.error() == e.error() } -> std::convertible_to<bool>;
    }
  {
    if (x.has_value()) {
      return false;
    }
    return x.error() == e.error();
  }
};

} // namespace pfn

#undef ASSERT

#ifdef INCLUDE_PFN_EXPECTED__POP_ASSERT
#pragma pop_macro("ASSERT")
#endif

#undef FWD

#ifdef INCLUDE_PFN_EXPECTED__POP_FWD
#pragma pop_macro("FWD")
#endif

#endif // INCLUDE_PFN_EXPECTED
