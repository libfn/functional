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

// [expected.bad], class template bad_expected_access
template <class E> class bad_expected_access;

// [expected.bad.void], specialization for void
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

// [expected.bad], primary template
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

// [expected.syn], unexpect_t disambiguation tag
struct unexpect_t {
  explicit unexpect_t() = default;
};
constexpr inline unexpect_t unexpect{};

// [expected.unexpected]
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

// [expected.unexpected], class template unexpected
template <class E> class unexpected {
  static_assert(detail::_is_valid_unexpected<E>);

public:
  // [expected.un.cons], constructors
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
  constexpr explicit unexpected(::std::in_place_t /*ignored*/,
                                Args &&...a) noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  template <class U, class... Args>
  constexpr explicit unexpected(::std::in_place_t /*ignored*/, ::std::initializer_list<U> i, Args &&...a) noexcept(
      ::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : e_(i, FWD(a)...)
  {
  }

  constexpr unexpected &operator=(unexpected const &) = default;
  constexpr unexpected &operator=(unexpected &&) = default;

  // [expected.un.general], observers
  constexpr E const &error() const & noexcept { return e_; };
  constexpr E &error() & noexcept { return e_; };
  constexpr E const &&error() const && noexcept { return ::std::move(e_); };
  constexpr E &&error() && noexcept { return ::std::move(e_); };

  // [expected.un.swap], swap
  constexpr void swap(unexpected &other) noexcept(::std::is_nothrow_swappable_v<E>)
  {
    static_assert(::std::is_swappable_v<E>);
    using ::std::swap;
    swap(e_, other.e_);
  }

  // [expected.un.eq], equality operator
  template <class E2>
  constexpr friend bool operator==(unexpected const &x, unexpected<E2> const &y) //
      noexcept(noexcept(detail::_implicit_to_bool(x.error() == y.error())))      // extension
  {
    return x.error() == y.error();
  }

  // [expected.un.swap], friend swap
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
// are defaulted iff both `T` and `E` are trivially copy/move-constructible.
template <class T, class E> union _storage_union_t {
  using _value_t = T;
  T v_;
  E e_;

  template <typename S>
  constexpr explicit _storage_union_t(bool s, S &&src) //
      noexcept(::std::is_nothrow_constructible_v<T, decltype(FWD(src).v_)>
               && ::std::is_nothrow_constructible_v<E, decltype(FWD(src).e_)>)
  {
    if (s)
      ::std::construct_at(::std::addressof(v_), FWD(src).v_);
    else
      ::std::construct_at(::std::addressof(e_), FWD(src).e_);
  }

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

  template <class... Args>
  constexpr explicit _storage_union_t(::std::in_place_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>)
    requires ::std::is_constructible_v<T, Args...>
      : v_(FWD(a)...)
  {
  }
  template <class... Args>
  constexpr explicit _storage_union_t(unexpect_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : e_(FWD(a)...)
  {
  }

  constexpr ~_storage_union_t() noexcept
    requires(::std::is_trivially_destructible_v<T> && ::std::is_trivially_destructible_v<E>)
  = default;
  constexpr ~_storage_union_t() noexcept {}

  // [expected.object.assign], implementation of reinit-expected
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

template <class E> union _storage_union_t<void, E> {
  // _dummy_t placeholder, so the union always has an active member
  struct _dummy_t final {
    constexpr _dummy_t() noexcept = default;
  };

  using _value_t = _dummy_t;
  _dummy_t v_;
  E e_;

  template <typename S>
  constexpr explicit _storage_union_t(bool s, S &&src) //
      noexcept(::std::is_nothrow_constructible_v<E, decltype(FWD(src).e_)>)
  {
    if (s)
      ::std::construct_at(::std::addressof(v_));
    else
      ::std::construct_at(::std::addressof(e_), FWD(src).e_);
  }

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

  constexpr explicit _storage_union_t(::std::in_place_t /*ignored*/) noexcept : v_{} {}

  template <class... Args>
  constexpr explicit _storage_union_t(unexpect_t /*ignored*/, Args &&...a) //
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

template <typename> constexpr bool _is_storage_union = false;
template <typename T, typename E> constexpr bool _is_storage_union<_storage_union_t<T, E>> = true;

// Shared implementation base class for ::pfn::expected, both primary template
// and void specialization. Members are public since inheritance is private.
template <class T, class E, class Policy> struct _storage {
  using _storage_t = _storage_union_t<T, E>;
  // `_value_t` is `T` for non-void, and trivial `_dummy_t` for void.
  using _value_t = _storage_t::_value_t;
  _storage_t storage_;
  bool set_;

  // Constraint traits used by the converting ctors from sibling classes,
  // as defined in [expected.object.cons] and [expected.void.cons].
  template <class U, class G, class UF, class GF>
  using _can_convert_detail = ::std::bool_constant<                                                        //
      ::std::is_void_v<T>                                                                                  //
          ? (::std::is_void_v<U> && ::std::is_constructible_v<E, GF>                                       //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G> &>       //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G>>         //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G> const &> //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G> const>)  //
          : (not(::std::is_same_v<T, U> && ::std::is_same_v<E, G>)                                         //
             && ::std::is_constructible_v<T, UF>                                                           //
             && ::std::is_constructible_v<E, GF>                                                           //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G> &>       //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G>>         //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G> const &> //
             && not ::std::is_constructible_v<unexpected<E>, typename Policy::template type<U, G> const>   //
             && (::std::is_same_v<bool, ::std::remove_cv_t<T>>                                             //
                 || (not ::std::is_constructible_v<T, typename Policy::template type<U, G> &>              //
                     && not ::std::is_constructible_v<T, typename Policy::template type<U, G>>             //
                     && not ::std::is_constructible_v<T, typename Policy::template type<U, G> const &>     //
                     && not ::std::is_constructible_v<T, typename Policy::template type<U, G> const>       //
                     && not ::std::is_convertible_v<typename Policy::template type<U, G> &, T>             //
                     && not ::std::is_convertible_v<typename Policy::template type<U, G> &&, T>            //
                     && not ::std::is_convertible_v<typename Policy::template type<U, G> const &, T>       //
                     && not ::std::is_convertible_v<typename Policy::template type<U, G> const &&, T>)))>;

  // For T=void, U will be void, so `add_lvalue_reference_t<add_const_t<U>>` used to make the trait below well-formed
  template <class U, class G>
  using _can_copy_convert = _can_convert_detail<U, G, ::std::add_lvalue_reference_t<::std::add_const_t<U>>, G const &>;
  template <class U, class G> using _can_move_convert = _can_convert_detail<U, G, U, G>;

  template <class U>
  using _can_convert = ::std::bool_constant<                                                  //
      not ::std::is_void_v<T>                                                                 //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, ::std::in_place_t>                    //
      && not ::std::is_same_v<::std::remove_cvref_t<U>, unexpect_t>                           // LWG4222
      && not ::std::is_same_v<typename Policy::template type<T, E>, ::std::remove_cvref_t<U>> //
      && not _is_some_unexpected<::std::remove_cvref_t<U>>                                    //
      && ::std::is_constructible_v<T, U>                                                      //
      && (not ::std::is_same_v<bool, ::std::remove_cv_t<T>>                                   //
          || not Policy::template is_specialization<::std::remove_cvref_t<U>>)>;

  // Constraint traits used by the converting assignment from sibling classes,
  // as defined in [expected.object.assign] and [expected.void.assign].
  //
  // For T=void, `_value_t` is the trivial `_dummy_t` so the `is_assignable_v<_value_t &, U>` substitution is
  // well-formed; the leading `not is_void_v<T>` guard then disables the trait.
  template <class U>
  using _can_convert_assign = ::std::bool_constant<                                           //
      not ::std::is_void_v<T>                                                                 //
      && not ::std::is_same_v<typename Policy::template type<T, E>, ::std::remove_cvref_t<U>> //
      && not _is_some_unexpected<::std::remove_cvref_t<U>>                                    //
      && ::std::is_constructible_v<_value_t, U>                                               //
      && ::std::is_assignable_v<_value_t &, U>                                                //
      && (::std::is_nothrow_constructible_v<_value_t, U>                                      //
          || ::std::is_nothrow_move_constructible_v<_value_t>                                 //
          || ::std::is_nothrow_move_constructible_v<E>)>;

  // Shared construction helper used by both the converting copy/move ctors
  // and same-type non-trivial copy/move ctors. Delegates the union's
  // member selection to _storage_union_t(bool, S&&), based on first parameter.
  template <typename S>
  constexpr explicit _storage(bool s, S &&src)
    requires(_is_storage_union<std::remove_cvref_t<S>>)
      : storage_(s, FWD(src)), set_(s)
  {
  }

  // The wrapper's default, value (U&&) and unexpected<G> converting ctors forward directly to the
  // in_place / unexpect overloads below, so no dedicated overloads for those are provided here.
  template <class... Args>
  constexpr explicit _storage(::std::in_place_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, Args...>
      : storage_(::std::in_place, FWD(a)...), set_(true)
  {
  }
  template <class U, class... Args>
  constexpr explicit _storage(::std::in_place_t /*ignored*/, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<_storage_t, ::std::in_place_t, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<_storage_t, ::std::in_place_t, ::std::initializer_list<U> &, Args...>
      : storage_(::std::in_place, il, FWD(a)...), set_(true)
  {
  }
  template <class... Args>
  constexpr explicit _storage(unexpect_t /*ignored*/, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<E, Args...>)
    requires ::std::is_constructible_v<E, Args...>
      : storage_(unexpect, FWD(a)...), set_(false)
  {
  }
  template <class U, class... Args>
  constexpr explicit _storage(unexpect_t /*ignored*/, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<E, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<E, ::std::initializer_list<U> &, Args...>
      : storage_(unexpect, il, FWD(a)...), set_(false)
  {
  }

  // `add_lvalue_reference_t<add_const_t<U>>` keeps the explicit-specifier
  // and noexcept substitution well-formed when U=void (yields `void const`).
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<::std::add_lvalue_reference_t<::std::add_const_t<U>>, T>
                     || not ::std::is_convertible_v<G const &, E>)
      _storage(typename Policy::template type<U, G> const &s) //
      noexcept((::std::is_void_v<T>
                || ::std::is_nothrow_constructible_v<_value_t, ::std::add_lvalue_reference_t<::std::add_const_t<U>>>)
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(_can_copy_convert<U, G>::value)
      : _storage(static_cast<_storage<U, G, Policy> const &>(s).set_,
                 static_cast<_storage<U, G, Policy> const &>(s).storage_)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, E>)
      _storage(typename Policy::template type<U, G> &&s)                               //
      noexcept((::std::is_void_v<T> || ::std::is_nothrow_constructible_v<_value_t, U>) //
               &&::std::is_nothrow_constructible_v<E, G>)                              // extension
    requires(_can_move_convert<U, G>::value)
      : _storage(static_cast<_storage<U, G, Policy> &&>(s).set_,
                 ::std::move(static_cast<_storage<U, G, Policy> &&>(s).storage_))
  {
  }

  // [expected.object.assign] and [expected.void.assign]: the converting `operator=(U&&)` and
  // `operator=(unexpected<G>...)` overloads are implemented directly in the wrappers (which call
  // `_assign_value` / `_assign_unexpected` below), so no dedicated overloads are provided here.
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

  constexpr _storage(_storage const &) = default;
  constexpr _storage(_storage &&) = default;
  constexpr _storage &operator=(_storage const &) = delete;
  constexpr _storage &operator=(_storage &&) = delete;

  constexpr ~_storage() //
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

  // [expected.object.obs], observers
  static constexpr auto &&_value(auto &&s) noexcept
    requires(not ::std::is_void_v<T>)
  {
    ASSERT(s.set_);
    return FWD(s).storage_.v_;
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
  // [expected.void.obs] for void specialization, operator* is a no-op that asserts the expected has a value
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

  static constexpr auto &&_error(auto &&s) noexcept
  {
    ASSERT(not s.set_);
    return FWD(s).storage_.e_;
  }
  constexpr E const &error() const & noexcept { return _error(*this); }
  constexpr E &error() & noexcept { return _error(*this); }
  constexpr E const &&error() const && noexcept { return _error(::std::move(*this)); }
  constexpr E &&error() && noexcept { return _error(::std::move(*this)); }

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

  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(_storage::_value(FWD(self)))>
               && ::std::is_nothrow_constructible_v<E, decltype(_storage::_error(FWD(self)))>)
    requires(not ::std::is_void_v<T> && ::std::is_invocable_v<Fn, decltype(_storage::_value(FWD(self)))>
             && ::std::is_constructible_v<E, decltype(_storage::_error(FWD(self)))>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(_storage::_value(FWD(self)))>>;
    static_assert(Policy::template is_specialization<result_t>);
    static_assert(::std::is_same_v<typename result_t::error_type, typename ::std::remove_cvref_t<Self>::error_type>);
    if (self.has_value()) {
      return ::std::invoke(FWD(fn), _storage::_value(FWD(self)));
    }
    return result_t(unexpect, _storage::_error(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn>
               && ::std::is_nothrow_constructible_v<E, decltype(_storage::_error(FWD(self)))>)
    requires(::std::is_void_v<T> && ::std::is_invocable_v<Fn>
             && ::std::is_constructible_v<E, decltype(_storage::_error(FWD(self)))>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn>>;
    static_assert(Policy::template is_specialization<result_t>);
    static_assert(::std::is_same_v<typename result_t::error_type, typename ::std::remove_cvref_t<Self>::error_type>);
    if (self.has_value()) {
      return ::std::invoke(FWD(fn));
    }
    return result_t(unexpect, _storage::_error(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>
               && ::std::is_nothrow_constructible_v<T, decltype(_storage::_value(FWD(self)))>)
    requires(not ::std::is_void_v<T> && ::std::is_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>
             && ::std::is_constructible_v<T, decltype(_storage::_value(FWD(self)))>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>;
    static_assert(Policy::template is_specialization<result_t>);
    static_assert(::std::is_same_v<typename result_t::value_type, typename ::std::remove_cvref_t<Self>::value_type>);
    if (self.has_value()) {
      return result_t(::std::in_place, _storage::_value(FWD(self)));
    }
    return ::std::invoke(FWD(fn), _storage::_error(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>)
    requires(::std::is_void_v<T> && ::std::is_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>)
  {
    using result_t = ::std::remove_cvref_t<::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>;
    static_assert(Policy::template is_specialization<result_t>);
    static_assert(::std::is_same_v<typename result_t::value_type, typename ::std::remove_cvref_t<Self>::value_type>);
    if (self.has_value()) {
      return result_t(::std::in_place);
    }
    return ::std::invoke(FWD(fn), _storage::_error(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(_storage::_value(FWD(self)))>
               && ::std::is_nothrow_constructible_v<E, decltype(_storage::_error(FWD(self)))>
               && (::std::is_void_v<::std::invoke_result_t<Fn, decltype(_storage::_value(FWD(self)))>>
                   || ::std::is_nothrow_constructible_v<
                       ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(_storage::_value(FWD(self)))>>,
                       ::std::invoke_result_t<Fn, decltype(_storage::_value(FWD(self)))>>))
    requires(not ::std::is_void_v<T> && ::std::is_invocable_v<Fn, decltype(_storage::_value(FWD(self)))>
             && ::std::is_constructible_v<E, decltype(_storage::_error(FWD(self)))>)
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(_storage::_value(FWD(self)))>>;
    static_assert(detail::_is_valid_expected<value_t, E>);
    using result_t = typename Policy::template type<value_t, E>;
    if (self.has_value()) {
      if constexpr (not ::std::is_void_v<value_t>) {
        static_assert(
            ::std::is_constructible_v<value_t, ::std::invoke_result_t<Fn, decltype(_storage::_value(FWD(self)))>>);
        return result_t(::std::in_place, ::std::invoke(FWD(fn), _storage::_value(FWD(self))));
      } else {
        ::std::invoke(FWD(fn), _storage::_value(FWD(self)));
        return result_t(::std::in_place);
      }
    }
    return result_t(unexpect, _storage::_error(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn>
               && ::std::is_nothrow_constructible_v<E, decltype(_storage::_error(FWD(self)))>
               && (::std::is_void_v<::std::invoke_result_t<Fn>>
                   || ::std::is_nothrow_constructible_v<::std::remove_cv_t<::std::invoke_result_t<Fn>>,
                                                        ::std::invoke_result_t<Fn>>))
    requires(::std::is_void_v<T> && ::std::is_invocable_v<Fn>
             && ::std::is_constructible_v<E, decltype(_storage::_error(FWD(self)))>)
  {
    using value_t = ::std::remove_cv_t<::std::invoke_result_t<Fn>>;
    static_assert(detail::_is_valid_expected<value_t, E>);
    using result_t = typename Policy::template type<value_t, E>;
    if (self.has_value()) {
      if constexpr (not ::std::is_void_v<value_t>) {
        static_assert(::std::is_constructible_v<value_t, ::std::invoke_result_t<Fn>>);
        return result_t(::std::in_place, ::std::invoke(FWD(fn)));
      } else {
        ::std::invoke(FWD(fn));
        return result_t(::std::in_place);
      }
    }
    return result_t(unexpect, _storage::_error(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>
               && ::std::is_nothrow_constructible_v<T, decltype(_storage::_value(FWD(self)))>
               && ::std::is_nothrow_constructible_v<
                   ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>,
                   ::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>)
    requires(not ::std::is_void_v<T> && ::std::is_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>
             && ::std::is_constructible_v<T, decltype(_storage::_value(FWD(self)))>)
  {
    using error_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>;
    static_assert(detail::_is_valid_unexpected<error_t>);
    static_assert(
        ::std::is_constructible_v<error_t, ::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>);
    using result_t = typename Policy::template type<T, error_t>;
    if (not self.has_value()) {
      return result_t(unexpect, ::std::invoke(FWD(fn), _storage::_error(FWD(self))));
    }
    return result_t(::std::in_place, _storage::_value(FWD(self)));
  }

  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::std::is_nothrow_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>
               && ::std::is_nothrow_constructible_v<
                   ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>,
                   ::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>)
    requires(::std::is_void_v<T> && ::std::is_invocable_v<Fn, decltype(_storage::_error(FWD(self)))>)
  {
    using error_t = ::std::remove_cv_t<::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>;
    static_assert(detail::_is_valid_unexpected<error_t>);
    static_assert(
        ::std::is_constructible_v<error_t, ::std::invoke_result_t<Fn, decltype(_storage::_error(FWD(self)))>>);
    using result_t = typename Policy::template type<void, error_t>;
    if (not self.has_value()) {
      return result_t(unexpect, ::std::invoke(FWD(fn), _storage::_error(FWD(self))));
    }
    return result_t(::std::in_place);
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

  // [expected.object.eq] and [expected.void.eq], equality operators
  template <class T2, class E2>
    requires(not ::std::is_void_v<T> && not ::std::is_void_v<T2>)
  constexpr friend bool operator==(typename Policy::template type<T, E> const &x,
                                   typename Policy::template type<T2, E2> const &y) //
      noexcept(noexcept(detail::_implicit_to_bool(*x == *y))
               && noexcept(detail::_implicit_to_bool(x.error() == y.error()))) // extension
    requires requires {
      { *x == *y } -> ::std::convertible_to<bool>;
      { x.error() == y.error() } -> ::std::convertible_to<bool>;
    }
  {
    if (x.has_value() != y.has_value())
      return false;
    if (x.has_value())
      return *x == *y;
    return x.error() == y.error();
  }
  template <class T2, class E2>
    requires(::std::is_void_v<T> && ::std::is_void_v<T2>)
  constexpr friend bool operator==(typename Policy::template type<T, E> const &x,
                                   typename Policy::template type<T2, E2> const &y) //
      noexcept(noexcept(detail::_implicit_to_bool(x.error() == y.error())))         // extension
    requires requires {
      { x.error() == y.error() } -> ::std::convertible_to<bool>;
    }
  {
    if (x.has_value() != y.has_value())
      return false;
    if (x.has_value())
      return true;
    return x.error() == y.error();
  }
  template <class T2>
    requires(not ::std::is_void_v<T> && not Policy::template is_specialization<T2>)
  constexpr friend bool operator==(typename Policy::template type<T, E> const &x, T2 const &v) //
      noexcept(noexcept(detail::_implicit_to_bool(*x == v)))                                   // extension
    requires requires {
      { *x == v } -> ::std::convertible_to<bool>;
    }
  {
    if (!x.has_value())
      return false;
    return *x == v;
  }
  template <class E2>
  constexpr friend bool operator==(typename Policy::template type<T, E> const &x, unexpected<E2> const &e) //
      noexcept(noexcept(detail::_implicit_to_bool(x.error() == e.error())))                                // extension
    requires requires {
      { x.error() == e.error() } -> ::std::convertible_to<bool>;
    }
  {
    if (x.has_value())
      return false;
    return x.error() == e.error();
  }

  // [expected.object.swap] and [expected.void.swap]
  constexpr friend void swap(typename Policy::template type<T, E> &x, //
                             typename Policy::template type<T, E> &y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }
};

} // namespace detail

template <class T, class E> class expected;
template <class E> class expected<void, E>;

namespace detail {
template <typename> constexpr bool _is_some_expected = false;
template <typename T, typename E> constexpr bool _is_some_expected<::pfn::expected<T, E>> = true;

struct expected_policy {
  template <class U, class G> using type = ::pfn::expected<U, G>;
  template <class X> static constexpr bool is_specialization = _is_some_expected<X>;
};

} // namespace detail

template <class T, class E> class expected : private detail::_storage<T, E, detail::expected_policy> {
  static_assert(detail::_is_valid_expected<T, E>);
  using _base = detail::_storage<T, E, detail::expected_policy>;

  // Allow sibling `_storage` instantiations to downcast into our private base
  // (needed for the lifted converting ctors and friend operators).
  template <class, class, class> friend struct detail::_storage;

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.object.cons] constructors. Not `using _base::_base;` to avoid a clang-16 bug.
  constexpr expected()                                       //
      noexcept(::std::is_nothrow_default_constructible_v<T>) // extension
    requires ::std::is_default_constructible_v<T>
      : _base(::std::in_place)
  {
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U const &, T> || not ::std::is_convertible_v<G const &, E>)
      expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &>
               && ::std::is_nothrow_constructible_v<E, G const &>) // extension
    requires(_base::template _can_copy_convert<U, G>::value)
      : _base(s)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, E>)
      expected(expected<U, G> &&s)                                                                 //
      noexcept(::std::is_nothrow_constructible_v<T, U> && ::std::is_nothrow_constructible_v<E, G>) // extension
    requires(_base::template _can_move_convert<U, G>::value)
      : _base(::std::move(s))
  {
  }
  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) expected(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)                 // extension
    requires(_base::template _can_convert<U>::value)
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

  // [expected.object.dtor]
  constexpr ~expected() = default;

  // [expected.void.assign], assignment. Not `using _base::operator=;` to avoid an MSVC bug.
  template <class U = T>
  constexpr expected &operator=(U &&s)                                                            //
      noexcept(::std::is_nothrow_assignable_v<T &, U> && ::std::is_nothrow_constructible_v<T, U>) // extension
    requires(_base::template _can_convert_assign<U>::value)
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
  constexpr auto and_then(F &&f) &                        //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                                    //
      noexcept(noexcept(_base::_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_and_then(::std::move(*this), FWD(f)))
  {
    return _base::_and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &                  //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                              //
      noexcept(noexcept(_base::_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_and_then(::std::move(*this), FWD(f)))
  {
    return _base::_and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                        //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
  {
    return _base::_or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                                    //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &                  //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
  {
    return _base::_or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                              //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                        //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform(::std::move(*this), FWD(f)))
  {
    return _base::_transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &                  //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform(::std::move(*this), FWD(f)))
  {
    return _base::_transform(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform_error(F &&f) &                        //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &                  //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }

  // [expected.object.eq], equality operators are hidden friends of _storage,
  // found via ADL (associated classes include the private base).
};

template <class E> class expected<void, E> : private detail::_storage<void, E, detail::expected_policy> {
  static_assert(detail::_is_valid_unexpected<E>);
  using _base = detail::_storage<void, E, detail::expected_policy>;

  // Allow sibling `_storage` instantiations to downcast into our private base
  // (needed for the lifted converting ctors and friend operators).
  template <class, class, class> friend struct detail::_storage;

public:
  using value_type = void;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  template <class U> using rebind = expected<U, error_type>;

  // [expected.void.cons], constructors. Not `using _base::_base;` to avoid a clang-16 bug.
  constexpr expected() noexcept : _base(::std::in_place) {}

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, E>) expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<E, G const &>)                                   // extension
    requires(_base::template _can_copy_convert<U, G>::value)
      : _base(s)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G, E>) expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<E, G>)                              // extension
    requires(_base::template _can_move_convert<U, G>::value)
      : _base(::std::move(s))
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

  // [expected.void.dtor]
  constexpr ~expected() = default;

  // [expected.void.assign], assignment. Not `using _base::operator=;` to avoid an MSVC bug.
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

  // [expected.void.emplace], emplace inherited from _storage
  using _base::emplace;

  // [expected.void.swap], swap; body delegates to _storage helper
  constexpr void swap(expected &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<E> && ::std::is_nothrow_swappable_v<E>)
    requires(::std::is_swappable_v<E> && ::std::is_move_constructible_v<E>)
  {
    this->_swap_with(rhs);
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
  constexpr auto and_then(F &&f) &                        //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) &&                                    //
      noexcept(noexcept(_base::_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_and_then(::std::move(*this), FWD(f)))
  {
    return _base::_and_then(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &                  //
      noexcept(noexcept(_base::_and_then(*this, FWD(f)))) // extension
      -> decltype(_base::_and_then(*this, FWD(f)))
  {
    return _base::_and_then(*this, FWD(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const &&                              //
      noexcept(noexcept(_base::_and_then(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_and_then(::std::move(*this), FWD(f)))
  {
    return _base::_and_then(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto or_else(F &&f) &                        //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
  {
    return _base::_or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) &&                                    //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &                  //
      noexcept(noexcept(_base::_or_else(*this, FWD(f)))) // extension
      -> decltype(_base::_or_else(*this, FWD(f)))
  {
    return _base::_or_else(*this, FWD(f));
  }
  template <class F>
  constexpr auto or_else(F &&f) const &&                              //
      noexcept(noexcept(_base::_or_else(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_or_else(::std::move(*this), FWD(f)))
  {
    return _base::_or_else(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform(F &&f) &                        //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform(::std::move(*this), FWD(f)))
  {
    return _base::_transform(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &                  //
      noexcept(noexcept(_base::_transform(*this, FWD(f)))) // extension
      -> decltype(_base::_transform(*this, FWD(f)))
  {
    return _base::_transform(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform(::std::move(*this), FWD(f)))
  {
    return _base::_transform(::std::move(*this), FWD(f));
  }

  template <class F>
  constexpr auto transform_error(F &&f) &                        //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) &&                                    //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &                  //
      noexcept(noexcept(_base::_transform_error(*this, FWD(f)))) // extension
      -> decltype(_base::_transform_error(*this, FWD(f)))
  {
    return _base::_transform_error(*this, FWD(f));
  }
  template <class F>
  constexpr auto transform_error(F &&f) const &&                              //
      noexcept(noexcept(_base::_transform_error(::std::move(*this), FWD(f)))) // extension
      -> decltype(_base::_transform_error(::std::move(*this), FWD(f)))
  {
    return _base::_transform_error(::std::move(*this), FWD(f));
  }

  // [expected.void.eq], equality operators are hidden friends of _storage,
  // found via ADL (associated classes include the private base).
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
