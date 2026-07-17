// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_EXPECTED
#define INCLUDE_FN_EXPECTED

#include <pfn/expected.hpp>
#include <pfn/utility.hpp>

#include <fn/detail/traits.hpp>
#include <fn/fwd.hpp>
#include <fn/pack.hpp>
#include <fn/sum.hpp>

#include <type_traits>
#include <utility>

namespace fn {

// Bring the C++23 polyfill primitives into `fn` namespace, for consistent type naming.
using ::pfn::bad_expected_access;
using ::pfn::unexpect;
using ::pfn::unexpect_t;
using ::pfn::unexpected;

template <typename T>
concept some_expected = detail::_some_expected<T>;

template <typename T>
concept some_expected_non_void = //
    some_expected<T>             //
    && !::std::is_same_v<void, typename ::std::remove_cvref_t<T>::value_type>;

template <typename T>
concept some_expected_void = //
    some_expected<T>         //
    && ::std::is_same_v<void, typename ::std::remove_cvref_t<T>::value_type>;

namespace detail {

struct expected_policy {
  template <class U, class G> using type = ::fn::expected<U, G>;
  template <class X> static constexpr bool is_specialization = _is_some_expected<X &>;
};

// Exposition-only probes for the noexcept specs below: the value/error type of an
// fn::expected, or a never-matching incomplete tag when X is not one.
struct _never_t;
template <typename X> struct _expected_types {
  using value_type = _never_t;
  using error_type = _never_t;
};
template <typename U, typename G> struct _expected_types<::fn::expected<U, G>> {
  using value_type = U;
  using error_type = G;
};

// A sum<> error is unconstructible, so an expected carrying one can never hold an error: every arm
// that lifts one is unreachable, and what cannot run cannot throw. Guarded on the SOURCE's error
// type - the value arms of the same expected still relocate, and still weigh.
template <typename E, typename Type, typename... Args>
constexpr inline bool _nothrow_error_arm = _nothrow_initializable<Type, Args...>;
template <typename E, typename Type, typename... Args>
  requires ::std::is_same_v<E, sum<>>
constexpr inline bool _nothrow_error_arm<E, Type, Args...> = true;

// Carrying the callback's value across into a widened result. An expected<void, ...> has no value to
// carry, and `declval<void>()` is not a thing to ask about.
template <typename Type, typename Src>
constexpr inline bool _nothrow_carry_value
    = _nothrow_initializable<Type, ::std::in_place_t, decltype(::std::declval<Src>().value())>;
template <typename Type, typename Src>
  requires ::std::is_void_v<typename ::std::remove_cvref_t<Src>::value_type>
constexpr inline bool _nothrow_carry_value<Type, Src> = _nothrow_initializable<Type, ::std::in_place_t>;

// `and_then` and `or_else` each have two arms - the callback's own expected is returned, or the two
// error (value) types are widened into a sum - and `if constexpr` picks between them. A
// noexcept-specifier is an ordinary constant expression and cannot pick: the untaken arm's spelling
// would have to be well-formed too. Hence a trait, whose constrained specializations mirror the
// body's arms. Both lead with `_is_some_expected`, so a callback returning something else leaves the
// unconstrained primary to answer, and the body's static_assert to diagnose - a specification must
// not pre-empt that with a hard error.
template <typename E, typename Fn, typename ErrArg, typename... ValArg> struct _nothrow_and_then : ::std::false_type {};

template <typename E, typename Fn, typename ErrArg, typename... ValArg>
  requires _is_some_expected<::std::remove_cvref_t<typename _apply_result<Fn, ValArg...>::type> &>
           && ::std::is_same_v<
               typename _expected_types<::std::remove_cvref_t<typename _apply_result<Fn, ValArg...>::type>>::error_type,
               E>
struct _nothrow_and_then<E, Fn, ErrArg, ValArg...>
    : ::std::bool_constant<
          _is_nothrow_applicable<Fn, ValArg...>::value // the callback
              && ::std::is_nothrow_constructible_v<::std::remove_cvref_t<typename _apply_result<Fn, ValArg...>::type>,
                                                   ::fn::unexpect_t, ErrArg>> {}; // lifting self's error

template <typename E, typename Fn, typename ErrArg, typename... ValArg>
  requires _is_some_expected<::std::remove_cvref_t<typename _apply_result<Fn, ValArg...>::type> &>
           && (not ::std::is_same_v<
               typename _expected_types<::std::remove_cvref_t<typename _apply_result<Fn, ValArg...>::type>>::error_type,
               E>)
struct _nothrow_and_then<E, Fn, ErrArg, ValArg...> {
  using type = ::std::remove_cvref_t<typename _apply_result<Fn, ValArg...>::type>;
  using new_type = ::fn::expected<typename type::value_type, sum_for<E, typename type::error_type>>;

  static constexpr bool value
      = _is_nothrow_applicable<Fn, ValArg...>::value // the callback
        && _nothrow_carry_value<new_type, type>      // carrying its value across
        && _nothrow_initializable<new_type, ::fn::unexpect_t,
                                  decltype(::std::declval<type>().error())> // widening its error
        && _nothrow_error_arm<E, new_type, ::fn::unexpect_t, ErrArg>;       // widening self's error
};

// or_else's arms, mirrored the same way. ValArg is the type of self's value as the body relocates it
// (spelled through apply_const_lvalue_t by the caller, since for a void T there is no value to name).
template <typename T, typename Fn, typename ErrArg, typename ValArg> struct _nothrow_or_else : ::std::false_type {};

template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires _is_some_expected<::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type> &>
           && ::std::is_same_v<
               typename _expected_types<::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>>::value_type, T>
struct _nothrow_or_else<T, Fn, ErrArg, ValArg>
    : ::std::bool_constant<
          _is_nothrow_applicable<Fn, ErrArg>::value // the callback
          && (::std::is_void_v<T>
              || _nothrow_initializable<::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>,
                                        ::std::in_place_t, ValArg>)> {}; // carrying self's value

template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires _is_some_expected<::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type> &>
           && (not ::std::is_same_v<
               typename _expected_types<::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>>::value_type,
               T>)
struct _nothrow_or_else<T, Fn, ErrArg, ValArg> {
  using type = ::std::remove_cvref_t<typename _apply_result<Fn, ErrArg>::type>;
  using new_type = ::fn::expected<sum_for<T, typename type::value_type>, typename type::error_type>;

  static constexpr bool value
      = _is_nothrow_applicable<Fn, ErrArg>::value                      // the callback
        && _nothrow_initializable<new_type, ::std::in_place_t, ValArg> // widening self's value
        && _nothrow_carry_value<new_type, type>                        // widening its value
        && _nothrow_initializable<new_type, ::fn::unexpect_t,
                                  decltype(::std::declval<type>().error())>; // carrying its error
};

// Storage layer for ::fn::expected. Inherits the standard-conformant base from
// pfn, then hides the four monadic static helpers with sum-widening variants
// that materialise their result via `expected_policy::template type<U, G>`.
// The transform/transform_error helpers hand pfn's _expected_from_invoke constructors a
// zero-argument thunk, so the result's member is direct-non-list-initialized from fn's own
// _apply (or sum::transform) result: no extra move, and immovable result types work.
// The statics carry the same extension noexcept as pfn's, computed through fn's own machinery: the
// callback of a sum/pack dispatch is invoked through `_apply`, not called directly, so it is
// `_is_nothrow_applicable` - not the std trait, which is false for a callable that is not directly
// applicable on a sum or a pack - that answers for it, and the widening arms are weighed by the
// traits above.
template <typename T, typename E> struct _expected_base : ::pfn::detail::_expected_base<T, E, expected_policy> {
  using _pfn_base = ::pfn::detail::_expected_base<T, E, expected_policy>;
  using _pfn_base::_pfn_base;

  // and_then, non-void value type
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_nothrow_and_then<E, Fn, decltype(_pfn_base::_error(FWD(self))),
                                               decltype(_pfn_base::_value(FWD(self)))>::value) // extension
    requires(not ::std::is_void_v<T>) && ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_value(FWD(self)))>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using type = typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_value(FWD(self)))>::type;
    static_assert(_is_some_expected<type &>);
    static_assert(::std::is_same_v<typename type::error_type, E> || some_sum<E>);
    if constexpr (::std::is_same_v<typename type::error_type, E>) {
      if (self.has_value())
        return ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)));
      else
        return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
    } else {
      using new_error_type = sum_for<E, typename type::error_type>;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (self.has_value()) {
        auto t = ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)));
        if (t.has_value())
          if constexpr (not ::std::is_void_v<typename new_type::value_type>)
            return new_type{::std::in_place, ::std::move(t).value()};
          else
            return new_type{::std::in_place};
        else
          return new_type{::fn::unexpect, ::std::move(t).error()};
      } else {
        if constexpr (not ::std::is_same_v<E, sum<>>)
          return new_type(::fn::unexpect, _pfn_base::_error(FWD(self)));
        else
          ::pfn::unreachable(); // LCOV_EXCL_LINE
      }
    }
  }

  // and_then, void value type
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn)                                               //
      noexcept(::fn::detail::_nothrow_and_then<E, Fn, decltype(_pfn_base::_error(FWD(self)))>::value) // extension
    requires(::std::is_void_v<T>) && ::fn::detail::_is_applicable<Fn>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using type = typename ::fn::detail::_apply_result<Fn>::type;
    static_assert(_is_some_expected<type &>);
    static_assert(::std::is_same_v<typename type::error_type, E> || some_sum<E>);
    if constexpr (::std::is_same_v<typename type::error_type, E>) {
      if (self.has_value())
        return ::fn::detail::_apply(FWD(fn));
      else
        return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
    } else {
      using new_error_type = sum_for<E, typename type::error_type>;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (self.has_value()) {
        auto t = ::fn::detail::_apply(FWD(fn));
        if (t.has_value())
          if constexpr (not ::std::is_void_v<typename new_type::value_type>)
            return new_type{::std::in_place, ::std::move(t).value()};
          else
            return new_type{::std::in_place};
        else
          return new_type{::fn::unexpect, ::std::move(t).error()};
      } else {
        if constexpr (not ::std::is_same_v<E, sum<>>)
          return new_type(::fn::unexpect, _pfn_base::_error(FWD(self)));
        else
          ::pfn::unreachable(); // LCOV_EXCL_LINE
      }
    }
  }

  // and_then, value type is the empty sum: a value can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, and the
  // result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&)                         //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires some_sum<T> && (::std::remove_cvref_t<T>::size == 0)
             && ::std::is_constructible_v<::fn::expected<T, E>, Self>
  {
    return FWD(self);
  }

  // or_else, covers both void and non-void value type. The value-copy conjunct spells the
  // _value(FWD(self)) category via apply_const_lvalue_t: unlike the requires clause below, a
  // noexcept operand is not constraint-checked, so the `is_void_v<T> ||` guard could not save
  // an ill-formed _value call (constrained away for void) from being a hard error.
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_nothrow_or_else<T, Fn, decltype(_pfn_base::_error(FWD(self))),
                                              ::fn::apply_const_lvalue_t<Self, typename _pfn_base::_value_t &&>>::value)
    requires ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_error(FWD(self)))>::value
             && (::std::is_void_v<T> || ::std::is_constructible_v<T, decltype(_pfn_base::_value(FWD(self)))>)
  {
    using type = typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_error(FWD(self)))>::type;
    static_assert(_is_some_expected<type &>);
    static_assert(::std::is_same_v<typename type::value_type, T> || some_sum<T>);
    if constexpr (::std::is_same_v<typename type::value_type, T>) {
      if (self.has_value())
        if constexpr (not ::std::is_void_v<T>)
          return type(::std::in_place, _pfn_base::_value(FWD(self)));
        else {
          static_assert(::std::is_void_v<typename type::value_type>);
#if defined(__clang__) && __clang_major__ <= 18
          // clang 15-18 miscompile the prvalue return below for three of the four Self ref-qualifier
          // instantiations (&, const &, const &&) at -O1/-O2: the value-state result is observed with
          // set_ == false (storage-poison). The workaround dodges the buggy mandatory copy-elision,
          // at the cost of a move -- an immovable error type must keep the prvalue (the workaround
          // would not compile; the miscompile is not observed in that shape).
          if constexpr (::std::is_move_constructible_v<type>)
            return ::std::move(type{::std::in_place});
          else
            return type{::std::in_place};
#else
          return type{::std::in_place};
#endif
        }
      else
        return ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)));
    } else {
      static_assert(not ::std::is_void_v<typename type::value_type>);
      using new_value_type = sum_for<T, typename type::value_type>;
      using new_type = ::fn::expected<new_value_type, typename type::error_type>;
      if (self.has_value())
        return new_type{::std::in_place, _pfn_base::_value(FWD(self))};
      else {
        auto t = ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)));
        if (t.has_value())
          return new_type{::std::in_place, ::std::move(t).value()};
        else
          return new_type{::fn::unexpect, ::std::move(t).error()};
      }
    }
  }

  // or_else, error type is the empty sum: an error can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, and the
  // result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&)                          //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires some_sum<E> && (::std::remove_cvref_t<E>::size == 0)
             && ::std::is_constructible_v<::fn::expected<T, E>, Self>
  {
    return FWD(self);
  }

  // transform, non-void value type, not a sum. In the noexcept specs of the transform and
  // transform_error overloads, only the apply and copying the untouched side can throw: the
  // new value/error is direct-non-list-initialized from the thunk's result (guaranteed elision).
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_value(FWD(self)))>::value
               && ::std::is_nothrow_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>) // extension
    requires(not ::std::is_void_v<T>) && (not some_sum<T>)
            && ::fn::detail::_is_applicable_if<not some_sum<T>, Fn, decltype(_pfn_base::_value(FWD(self)))>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using new_value_type = typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_value(FWD(self)))>::type;
    using type = ::fn::expected<new_value_type, E>;
    if (self.has_value())
      if constexpr (::std::is_void_v<new_value_type>) {
        ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)));
        return type();
      } else
        return type(::pfn::detail::_expected_from_invoke, ::std::in_place, [&fn, &self]() -> decltype(auto) {
          return ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)));
        });
    else
      return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
  }

  // transform, value type is a sum (delegates to sum::transform). The callback is constrained here,
  // in the immediate context, for the reason given on optional's sum-case _transform.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(noexcept(_pfn_base::_value(FWD(self)).transform(FWD(fn)))
               && ::std::is_nothrow_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>) // extension
    requires some_sum<T> && (::std::remove_cvref_t<T>::size > 0)
             && ::fn::detail::_typelist_applicable<Fn, decltype(_pfn_base::_value(FWD(self)))>
             && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using new_value_type = decltype(_pfn_base::_value(FWD(self)).transform(FWD(fn)));
    using type = ::fn::expected<new_value_type, E>;
    if (self.has_value())
      if constexpr (::std::is_void_v<new_value_type>) {
        _pfn_base::_value(FWD(self)).transform(FWD(fn));
        return type();
      } else
        return type(::pfn::detail::_expected_from_invoke, ::std::in_place,
                    [&fn, &self]() -> decltype(auto) { return _pfn_base::_value(FWD(self)).transform(FWD(fn)); });
    else
      return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
  }

  // transform, value type is the empty sum: a value can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, the mapping
  // is the identity and the result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&)                        //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires some_sum<T> && (::std::remove_cvref_t<T>::size == 0)
             && ::std::is_constructible_v<::fn::expected<T, E>, Self>
  {
    return FWD(self);
  }

  // transform, void value type
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn>::value
               && ::std::is_nothrow_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>) // extension
    requires(::std::is_void_v<T>) && ::fn::detail::_is_applicable<Fn>::value
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>
  {
    using new_value_type = typename ::fn::detail::_apply_result<Fn>::type;
    using type = ::fn::expected<new_value_type, E>;
    if (self.has_value())
      if constexpr (::std::is_void_v<new_value_type>) {
        ::fn::detail::_apply(FWD(fn));
        return type();
      } else
        return type(::pfn::detail::_expected_from_invoke, ::std::in_place,
                    [&fn]() -> decltype(auto) { return ::fn::detail::_apply(FWD(fn)); });
    else
      return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
  }

  // transform_error, error type is not a sum (the value-copy conjunct is spelled via
  // apply_const_lvalue_t for the same reason as _or_else's above)
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_error(FWD(self)))>::value
               && (::std::is_void_v<T>
                   || ::std::is_nothrow_constructible_v<
                       T, ::fn::apply_const_lvalue_t<Self, typename _pfn_base::_value_t &&>>)) // extension
    requires(not some_sum<E>)
            && ::fn::detail::_is_applicable_if<not some_sum<E>, Fn, decltype(_pfn_base::_error(FWD(self)))>::value
            && (::std::is_void_v<T> || ::std::is_constructible_v<T, decltype(_pfn_base::_value(FWD(self)))>)
  {
    using new_error_type = typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_error(FWD(self)))>::type;
    using type = ::fn::expected<T, new_error_type>;
    if (self.has_value())
      if constexpr (not ::std::is_void_v<T>)
        return type(::std::in_place, _pfn_base::_value(FWD(self)));
      else
        return type();
    else
      return type(::pfn::detail::_expected_from_invoke, ::fn::unexpect, [&fn, &self]() -> decltype(auto) {
        return ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)));
      });
  }

  // transform_error, error type is a sum (delegates to sum::transform). The callback is constrained
  // here, in the immediate context, for the reason given on optional's sum-case _transform.
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(noexcept(_pfn_base::_error(FWD(self)).transform(FWD(fn)))
               && (::std::is_void_v<T>
                   || ::std::is_nothrow_constructible_v<
                       T, ::fn::apply_const_lvalue_t<Self, typename _pfn_base::_value_t &&>>)) // extension
    requires some_sum<E> && (::std::remove_cvref_t<E>::size > 0)
             && ::fn::detail::_typelist_applicable<Fn, decltype(_pfn_base::_error(FWD(self)))>
             && (::std::is_void_v<T> || ::std::is_constructible_v<T, decltype(_pfn_base::_value(FWD(self)))>)
  {
    using new_error_type = decltype(_pfn_base::_error(FWD(self)).transform(FWD(fn)));
    using type = ::fn::expected<T, new_error_type>;
    if (self.has_value())
      if constexpr (not ::std::is_void_v<T>)
        return type(::std::in_place, _pfn_base::_value(FWD(self)));
      else
        return type();
    else
      return type(::pfn::detail::_expected_from_invoke, ::fn::unexpect,
                  [&fn, &self]() -> decltype(auto) { return _pfn_base::_error(FWD(self)).transform(FWD(fn)); });
  }

  // apply: elimination over both states, both arms required outright - each arm eliminates its
  // side's value through fn's own _apply (a pack or tuple-like payload by elements, a sum by
  // dispatch); a void value arm is invoked without a value.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value
               && ::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_error(FWD(self))),
                                                       Args...>::value) // extension
      -> decltype(auto)
    requires(not ::std::is_void_v<T>)
            && ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value
            && ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value
  {
    // Both arms are viable here, so they must yield the same result type.
    static_assert(::std::is_same_v<
                  typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::type,
                  typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::type>);
    if (self.has_value())
      return ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    else
      return ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // apply, void value type
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, Args...>::value
               && ::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_error(FWD(self))),
                                                       Args...>::value) // extension
      -> decltype(auto)
    requires ::std::is_void_v<T> && ::fn::detail::_is_applicable<Fn, Args...>::value
             && ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value
  {
    // Both arms are viable here, so they must yield the same result type.
    static_assert(::std::is_same_v<
                  typename ::fn::detail::_apply_result<Fn, Args...>::type,
                  typename ::fn::detail::_apply_result<Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::type>);
    if (self.has_value())
      return ::fn::detail::_apply(FWD(fn), FWD(args)...);
    else
      return ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value
               && ::fn::detail::_is_nothrow_applicable_r<Ret, Fn, decltype(_pfn_base::_error(FWD(self))),
                                                         Args...>::value) // extension
      -> Ret
    requires(not ::std::is_void_v<T>)
            && ::fn::detail::_is_applicable_r<Ret, Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value
            && ::fn::detail::_is_applicable_r<Ret, Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value
  {
    if (self.has_value())
      return ::fn::detail::_apply_r<Ret>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    else
      return ::fn::detail::_apply_r<Ret>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // apply_r, void value type
  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, Args...>::value
               && ::fn::detail::_is_nothrow_applicable_r<Ret, Fn, decltype(_pfn_base::_error(FWD(self))),
                                                         Args...>::value) // extension
      -> Ret
    requires ::std::is_void_v<T> && ::fn::detail::_is_applicable_r<Ret, Fn, Args...>::value
             && ::fn::detail::_is_applicable_r<Ret, Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value
  {
    if (self.has_value())
      return ::fn::detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
    else
      return ::fn::detail::_apply_r<Ret>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // apply_type: the tagged form - the value arm receives ::std::in_place followed by the value as
  // _apply_tagged hands it over (the tag alone when T is void; a tuple-like value's elements form
  // is the row's one signature), the error arm ::fn::unexpect followed by the error; the tags
  // never interconvert, so the dispatch stays airtight even where T and E do. Tags are passed as
  // prvalues, the exact shape the traits ask about; trailing arguments follow either arm's content.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)),
                                                                       FWD(args)...))
               && noexcept(::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                         FWD(args)...))) // extension
      -> decltype(auto)
    requires(not ::std::is_void_v<T>) && requires {
      ::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    } && requires {
      ::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
    }
  {
    // Both arms are viable here, so they must yield the same result type.
    static_assert(::std::is_same_v<decltype(::fn::detail::_apply_tagged<::std::in_place_t>(
                                       FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...)),
                                   decltype(::fn::detail::_apply_tagged<::fn::unexpect_t>(
                                       FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...))>);
    if (self.has_value())
      return ::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    else
      return ::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // apply_type, void value type
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, ::std::in_place_t, Args &&...>::value
               && noexcept(::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                         FWD(args)...))) // extension
      -> decltype(auto)
    requires ::std::is_void_v<T> && ::fn::detail::_is_applicable<Fn, ::std::in_place_t, Args &&...>::value && requires {
      ::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
    }
  {
    // Both arms are viable here, so they must yield the same result type.
    static_assert(::std::is_same_v<typename ::fn::detail::_apply_result<Fn, ::std::in_place_t, Args &&...>::type,
                                   decltype(::fn::detail::_apply_tagged<::fn::unexpect_t>(
                                       FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...))>);
    if (self.has_value())
      return ::fn::detail::_apply(FWD(fn), ::std::in_place_t{}, FWD(args)...);
    else
      return ::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)),
                                                                              FWD(args)...))
               && noexcept(::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                                FWD(args)...))) // extension
      -> Ret
    requires(not ::std::is_void_v<T>) && requires {
      ::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    } && requires {
      ::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
    }
  {
    if (self.has_value())
      return ::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    else
      return ::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // apply_type_r, void value type
  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, ::std::in_place_t, Args &&...>::value
               && noexcept(::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                                FWD(args)...))) // extension
      -> Ret
    requires ::std::is_void_v<T> && ::fn::detail::_is_applicable_r<Ret, Fn, ::std::in_place_t, Args &&...>::value
             && requires {
                  ::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                       FWD(args)...);
                }
  {
    if (self.has_value())
      return ::fn::detail::_apply_r<Ret>(FWD(fn), ::std::in_place_t{}, FWD(args)...);
    else
      return ::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // transform_error, error type is the empty sum: an error can never be constructed, so the
  // callback can never be presented one - it is left alone, not invoked and not even instantiated,
  // the mapping is the identity and the result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&)                  //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires some_sum<E> && (::std::remove_cvref_t<E>::size == 0)
             && ::std::is_constructible_v<::fn::expected<T, E>, Self>
  {
    return FWD(self);
  }
};

} // namespace detail

// Primary template - non-void value type
template <typename T, typename Err> class expected : private detail::_expected_base<T, Err> {
  using _base = detail::_expected_base<T, Err>;

  // Allow sibling _expected_base instantiations to downcast into the private base.
  template <class, class, class> friend struct ::pfn::detail::_expected_base;
  template <class, class> friend struct ::fn::detail::_expected_base;

public:
  using value_type = T;
  using error_type = Err;
  using unexpected_type = ::fn::unexpected<Err>;

  template <class U> using rebind = expected<U, error_type>;

  // Constructors. Explicit forwarders to the base mirror pfn::expected.
  constexpr expected() noexcept(::std::is_nothrow_default_constructible_v<T>)
    requires ::std::is_default_constructible_v<T>
      : _base(::std::in_place)
  {
  }

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U const &, T> || not ::std::is_convertible_v<G const &, Err>)
      expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<T, U const &> && ::std::is_nothrow_constructible_v<Err, G const &>)
    requires(_base::template _can_copy_convert<U, G>::value)
      : _base(s)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<U, T> || not ::std::is_convertible_v<G, Err>)
      expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<T, U> && ::std::is_nothrow_constructible_v<Err, G>)
    requires(_base::template _can_move_convert<U, G>::value)
      : _base(::std::move(s))
  {
  }
  template <class U = ::std::remove_cv_t<T>>
  constexpr explicit(not ::std::is_convertible_v<U, T>) expected(U &&v) //
      noexcept(::std::is_nothrow_constructible_v<T, U>)
    requires(_base::template _can_convert<U>::value)
      : _base(::std::in_place, FWD(v))
  {
  }

  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, Err>) expected(::fn::unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &>)
      : _base(::fn::unexpect, ::std::forward<G const &>(g.error()))
  {
  }
  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G, Err>) expected(::fn::unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G>)
      : _base(::fn::unexpect, ::std::forward<G>(g.error()))
  {
  }

  template <class... Args>
  constexpr explicit expected(::std::in_place_t, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, Args...>)
    requires ::std::is_constructible_v<T, Args...>
      : _base(::std::in_place, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::std::in_place_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<T, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<T, ::std::initializer_list<U> &, Args...>
      : _base(::std::in_place, il, FWD(a)...)
  {
  }
  template <class... Args>
  constexpr explicit expected(::fn::unexpect_t, Args &&...a)    //
      noexcept(::std::is_nothrow_constructible_v<Err, Args...>) //
    requires ::std::is_constructible_v<Err, Args...>
      : _base(::fn::unexpect, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::fn::unexpect_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<Err, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<Err, ::std::initializer_list<U> &, Args...>
      : _base(::fn::unexpect, il, FWD(a)...)
  {
  }

  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &s) //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<Err>
             && ::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_constructible_v<Err>)
  = default;
  constexpr expected(expected const &s) //
      noexcept(::std::is_nothrow_copy_constructible_v<T> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_constructible_v<T> && ::std::is_copy_constructible_v<Err>
             && (not ::std::is_trivially_copy_constructible_v<T> || not ::std::is_trivially_copy_constructible_v<Err>))
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr expected(expected &&s) noexcept
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<Err>
             && ::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_constructible_v<Err>)
  = default;
  constexpr expected(expected &&s) //
      noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_constructible_v<Err>
             && (not ::std::is_trivially_move_constructible_v<T> || not ::std::is_trivially_move_constructible_v<Err>))
      : _base(s.set_, FWD(s).storage_)
  {
  }

  constexpr ~expected() = default;

  // Assignment. Explicit forwarders mirror pfn::expected to avoid an MSVC bug.
  template <class U = T>
  constexpr expected &operator=(U &&s) //
      noexcept(::std::is_nothrow_assignable_v<T &, U> && ::std::is_nothrow_constructible_v<T, U>)
    requires(_base::template _can_convert_assign<U>::value)
  {
    this->_assign_value(FWD(s));
    return *this;
  }
  template <class G>
  constexpr expected &operator=(::fn::unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G const &> && ::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &> && ::std::is_assignable_v<Err &, G const &>
             && (::std::is_nothrow_constructible_v<Err, G const &> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_assign_unexpected(s);
    return *this;
  }
  template <class G>
  constexpr expected &operator=(::fn::unexpected<G> &&s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G> && ::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G> && ::std::is_assignable_v<Err &, G>
             && (::std::is_nothrow_constructible_v<Err, G> || ::std::is_nothrow_move_constructible_v<T>
                 || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_assign_unexpected(::std::move(s));
    return *this;
  }
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &) //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>
               && ::std::is_nothrow_copy_assignable_v<Err> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_assignable_v<T> && ::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<Err>
             && ::std::is_copy_constructible_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>)
             && ::std::is_trivially_copy_constructible_v<T> && ::std::is_trivially_copy_assignable_v<T>
             && ::std::is_trivially_destructible_v<T> && ::std::is_trivially_copy_constructible_v<Err>
             && ::std::is_trivially_copy_assignable_v<Err> && ::std::is_trivially_destructible_v<Err>)
  = default;
  constexpr expected &operator=(expected const &s) //
      noexcept(::std::is_nothrow_copy_assignable_v<T> && ::std::is_nothrow_copy_constructible_v<T>
               && ::std::is_nothrow_copy_assignable_v<Err> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_assignable_v<T> && ::std::is_copy_constructible_v<T> && ::std::is_copy_assignable_v<Err>
             && ::std::is_copy_constructible_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>)
             && (not ::std::is_trivially_copy_constructible_v<T> || not ::std::is_trivially_copy_assignable_v<T>
                 || not ::std::is_trivially_destructible_v<T> || not ::std::is_trivially_copy_constructible_v<Err>
                 || not ::std::is_trivially_copy_assignable_v<Err> || not ::std::is_trivially_destructible_v<Err>))
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr expected &operator=(expected &&) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>
               && ::std::is_nothrow_move_assignable_v<Err> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T> && ::std::is_move_constructible_v<Err>
             && ::std::is_move_assignable_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>)
             && ::std::is_trivially_move_constructible_v<T> && ::std::is_trivially_move_assignable_v<T>
             && ::std::is_trivially_destructible_v<T> && ::std::is_trivially_move_constructible_v<Err>
             && ::std::is_trivially_move_assignable_v<Err> && ::std::is_trivially_destructible_v<Err>)
  = default;
  constexpr expected &operator=(expected &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<T> && ::std::is_nothrow_move_constructible_v<T>
               && ::std::is_nothrow_move_assignable_v<Err> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<T> && ::std::is_move_assignable_v<T> && ::std::is_move_constructible_v<Err>
             && ::std::is_move_assignable_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>)
             && (not ::std::is_trivially_move_constructible_v<T> || not ::std::is_trivially_move_assignable_v<T>
                 || not ::std::is_trivially_destructible_v<T> || not ::std::is_trivially_move_constructible_v<Err>
                 || not ::std::is_trivially_move_assignable_v<Err> || not ::std::is_trivially_destructible_v<Err>))
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  // Observers inherited from _expected_base
  using _base::operator*;
  using _base::operator->;
  using _base::operator bool;
  using _base::error;
  using _base::error_or;
  using _base::has_error;
  using _base::has_value;
  using _base::value;
  using _base::value_or;

  // Emplace inherited from _expected_base
  using _base::emplace;

  // Swap; body delegates to _expected_base helper
  constexpr void
  swap(expected &rhs) noexcept(::std::is_nothrow_move_constructible_v<T> && ::std::is_nothrow_swappable_v<T>
                               && ::std::is_nothrow_move_constructible_v<Err> && ::std::is_nothrow_swappable_v<Err>)
    requires(::std::is_swappable_v<T> && ::std::is_swappable_v<Err> && ::std::is_move_constructible_v<T>
             && ::std::is_move_constructible_v<Err>
             && (::std::is_nothrow_move_constructible_v<T> || ::std::is_nothrow_move_constructible_v<Err>))
  {
    this->_swap_with(rhs);
  }

  // Elimination over both states, mirroring sum's apply family: each arm takes its side's value
  // unpacked as fn::apply would hand it over, keyed (apply_type) by the constructor tag naming
  // the state - ::std::in_place for the value, ::fn::unexpect for the error. Bodies delegate to
  // _expected_base static helpers.
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) &        //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) &&                    //
      noexcept(noexcept(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const &  //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const &&              //
      noexcept(noexcept(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) &                      //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) &&                                  //
      noexcept(noexcept(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const &                //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const &&                            //
      noexcept(noexcept(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) &        //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) &&                    //
      noexcept(noexcept(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply_type(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const &  //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const &&              //
      noexcept(noexcept(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply_type(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) &                      //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) &&                                  //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const &                //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const &&                            //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }

  // Monadic operations. Bodies delegate to _expected_base static helpers, which perform sum-widening.
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

  // Convert to graded monad. A lifting overload wraps one side in a sum and relocates the other
  // untouched, so it weighs both; the ones whose side already is a sum only return *this.
  constexpr auto sum_error() const & noexcept(::std::is_nothrow_constructible_v<value_type, value_type const &>
                                              && ::std::is_nothrow_constructible_v<sum<error_type>, error_type const &>
                                              && ::std::is_nothrow_move_constructible_v<sum<error_type>>) // extension
      -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{::std::in_place, this->value()};
    else
      return type{::fn::unexpect, sum<error_type>(this->error())};
  }
  constexpr auto sum_error() && noexcept(::std::is_nothrow_constructible_v<value_type, value_type>
                                         && ::std::is_nothrow_constructible_v<sum<error_type>, error_type>
                                         && ::std::is_nothrow_move_constructible_v<sum<error_type>>) // extension
      -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{::std::in_place, ::std::move(*this).value()};
    else
      return type{::fn::unexpect, sum<error_type>(::std::move(*this).error())};
  }
  constexpr auto sum_error() & noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return *this;
  }
  constexpr auto sum_error() const & noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return *this;
  }
  constexpr auto sum_error() && noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto sum_error() const && noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return ::std::move(*this);
  }

  constexpr auto
  sum_value() const & noexcept(::std::is_nothrow_constructible_v<sum<value_type>, value_type const &>
                               && ::std::is_nothrow_move_constructible_v<sum<value_type>>
                               && ::std::is_nothrow_constructible_v<error_type, error_type const &>) // extension
      -> expected<sum<value_type>, error_type>
    requires(not some_sum<value_type>)
  {
    using type = expected<sum<value_type>, error_type>;
    if (this->has_value())
      return type{::std::in_place, sum<value_type>(this->value())};
    else
      return type{::fn::unexpect, this->error()};
  }
  constexpr auto sum_value() && noexcept(::std::is_nothrow_constructible_v<sum<value_type>, value_type>
                                         && ::std::is_nothrow_move_constructible_v<sum<value_type>>
                                         && ::std::is_nothrow_constructible_v<error_type, error_type>) // extension
      -> expected<sum<value_type>, error_type>
    requires(not some_sum<value_type>)
  {
    using type = expected<sum<value_type>, error_type>;
    if (this->has_value())
      return type{::std::in_place, sum<value_type>(::std::move(*this).value())};
    else
      return type{::fn::unexpect, ::std::move(*this).error()};
  }
  constexpr auto sum_value() & noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return *this;
  }
  constexpr auto sum_value() const & noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return *this;
  }
  constexpr auto sum_value() && noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto sum_value() const && noexcept -> decltype(auto)
    requires(some_sum<value_type>)
  {
    return ::std::move(*this);
  }

private:
  // Direct-non-list-initializes the value (in_place) or error (unexpect) member from the
  // result of a callable; used by the monadic functions implemented in _expected_base.
  template <class Tag, class Fn, class... Args>
  constexpr explicit expected(::pfn::detail::_expected_from_invoke_t tag, Tag which, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_base, ::pfn::detail::_expected_from_invoke_t, Tag, Fn, Args...>)
      : _base(tag, which, FWD(fn), FWD(args)...)
  {
  }
};

template <typename Err> class expected<void, Err> : private detail::_expected_base<void, Err> {
  using _base = detail::_expected_base<void, Err>;

  template <class, class, class> friend struct ::pfn::detail::_expected_base;
  template <class, class> friend struct ::fn::detail::_expected_base;

public:
  using value_type = void;
  using error_type = Err;
  using unexpected_type = ::fn::unexpected<Err>;

  template <class U> using rebind = expected<U, error_type>;

  constexpr expected() noexcept : _base(::std::in_place) {}

  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, Err>) expected(expected<U, G> const &s) //
      noexcept(::std::is_nothrow_constructible_v<Err, G const &>)
    requires(_base::template _can_copy_convert<U, G>::value)
      : _base(s)
  {
  }
  template <class U, class G>
  constexpr explicit(not ::std::is_convertible_v<G, Err>) expected(expected<U, G> &&s) //
      noexcept(::std::is_nothrow_constructible_v<Err, G>)
    requires(_base::template _can_move_convert<U, G>::value)
      : _base(::std::move(s))
  {
  }
  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G const &, Err>) expected(::fn::unexpected<G> const &g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &>)
      : _base(::fn::unexpect, ::std::forward<G const &>(g.error()))
  {
  }
  template <class G>
  constexpr explicit(not ::std::is_convertible_v<G, Err>) expected(::fn::unexpected<G> &&g) //
      noexcept(::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G>)
      : _base(::fn::unexpect, ::std::forward<G>(g.error()))
  {
  }

  constexpr explicit expected(::std::in_place_t) noexcept : _base(::std::in_place) {}

  template <class... Args>
  constexpr explicit expected(::fn::unexpect_t, Args &&...a)    //
      noexcept(::std::is_nothrow_constructible_v<Err, Args...>) //
    requires ::std::is_constructible_v<Err, Args...>
      : _base(::fn::unexpect, FWD(a)...)
  {
  }
  template <class U, class... Args>
  constexpr explicit expected(::fn::unexpect_t, ::std::initializer_list<U> il, Args &&...a) //
      noexcept(::std::is_nothrow_constructible_v<Err, ::std::initializer_list<U> &, Args...>)
    requires ::std::is_constructible_v<Err, ::std::initializer_list<U> &, Args...>
      : _base(::fn::unexpect, il, FWD(a)...)
  {
  }

  constexpr expected(expected const &) = delete;
  constexpr expected(expected const &)
    requires(::std::is_copy_constructible_v<Err> && ::std::is_trivially_copy_constructible_v<Err>)
  = default;
  constexpr expected(expected const &s) //
      noexcept(::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_constructible_v<Err> && not ::std::is_trivially_copy_constructible_v<Err>)
      : _base(s.set_, FWD(s).storage_)
  {
  }
  constexpr expected(expected &&s) noexcept
    requires(::std::is_move_constructible_v<Err> && ::std::is_trivially_move_constructible_v<Err>)
  = default;
  constexpr expected(expected &&s) //
      noexcept(::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<Err> && not ::std::is_trivially_move_constructible_v<Err>)
      : _base(s.set_, FWD(s).storage_)
  {
  }

  constexpr ~expected() = default;

  template <class G>
  constexpr expected &operator=(::fn::unexpected<G> const &s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G const &> && ::std::is_nothrow_constructible_v<Err, G const &>)
    requires(::std::is_constructible_v<Err, G const &> && ::std::is_assignable_v<Err &, G const &>)
  {
    this->_assign_unexpected(s);
    return *this;
  }
  template <class G>
  constexpr expected &operator=(::fn::unexpected<G> &&s) //
      noexcept(::std::is_nothrow_assignable_v<Err &, G> && ::std::is_nothrow_constructible_v<Err, G>)
    requires(::std::is_constructible_v<Err, G> && ::std::is_assignable_v<Err &, G>)
  {
    this->_assign_unexpected(::std::move(s));
    return *this;
  }
  constexpr expected &operator=(expected const &) = delete;
  constexpr expected &operator=(expected const &) //
      noexcept(::std::is_nothrow_copy_assignable_v<Err> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_assignable_v<Err> && ::std::is_copy_constructible_v<Err>
             && ::std::is_trivially_copy_constructible_v<Err> && ::std::is_trivially_copy_assignable_v<Err>
             && ::std::is_trivially_destructible_v<Err>)
  = default;
  constexpr expected &operator=(expected const &s) //
      noexcept(::std::is_nothrow_copy_assignable_v<Err> && ::std::is_nothrow_copy_constructible_v<Err>)
    requires(::std::is_copy_assignable_v<Err> && ::std::is_copy_constructible_v<Err>
             && (not ::std::is_trivially_copy_constructible_v<Err> || not ::std::is_trivially_copy_assignable_v<Err>
                 || not ::std::is_trivially_destructible_v<Err>))
  {
    this->_assign(static_cast<_base const &>(s));
    return *this;
  }
  constexpr expected &operator=(expected &&) //
      noexcept(::std::is_nothrow_move_assignable_v<Err> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<Err> && ::std::is_move_assignable_v<Err>
             && ::std::is_trivially_move_constructible_v<Err> && ::std::is_trivially_move_assignable_v<Err>
             && ::std::is_trivially_destructible_v<Err>)
  = default;
  constexpr expected &operator=(expected &&s) //
      noexcept(::std::is_nothrow_move_assignable_v<Err> && ::std::is_nothrow_move_constructible_v<Err>)
    requires(::std::is_move_constructible_v<Err> && ::std::is_move_assignable_v<Err>
             && (not ::std::is_trivially_move_constructible_v<Err> || not ::std::is_trivially_move_assignable_v<Err>
                 || not ::std::is_trivially_destructible_v<Err>))
  {
    this->_assign(static_cast<_base &&>(s));
    return *this;
  }

  using _base::emplace;

  constexpr void swap(expected &rhs) //
      noexcept(::std::is_nothrow_move_constructible_v<Err> && ::std::is_nothrow_swappable_v<Err>)
    requires(::std::is_swappable_v<Err> && ::std::is_move_constructible_v<Err>)
  {
    this->_swap_with(rhs);
  }

  using _base::operator*;
  using _base::operator bool;
  using _base::error;
  using _base::error_or;
  using _base::has_error;
  using _base::has_value;
  using _base::value;

  // Elimination over both states, mirroring sum's apply family: the value arm takes no value
  // (apply) or the ::std::in_place tag alone (apply_type), the error arm the error unpacked as
  // fn::apply would hand it over, after ::fn::unexpect in the tagged form. Bodies delegate to
  // _expected_base static helpers.
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) &        //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) &&                    //
      noexcept(noexcept(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const &  //
      noexcept(noexcept(_base::_apply(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply(F &&f, Args &&...args) const &&              //
      noexcept(noexcept(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) &                      //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) &&                                  //
      noexcept(noexcept(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const &                //
      noexcept(noexcept(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_r(F &&f, Args &&...args) const &&                            //
      noexcept(noexcept(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) &        //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) &&                    //
      noexcept(noexcept(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply_type(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const &  //
      noexcept(noexcept(_base::_apply_type(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(*this, FWD(f), FWD(args)...))
  {
    return _base::_apply_type(*this, FWD(f), FWD(args)...);
  }
  template <class F, class... Args>
  [[nodiscard]] constexpr auto apply_type(F &&f, Args &&...args) const &&              //
      noexcept(noexcept(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::_apply_type(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::_apply_type(::std::move(*this), FWD(f), FWD(args)...);
  }

  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) &                      //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) &&                                  //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const &                //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(*this, FWD(f), FWD(args)...);
  }
  template <class Ret, class F, class... Args>
  [[nodiscard]] constexpr auto apply_type_r(F &&f, Args &&...args) const &&                            //
      noexcept(noexcept(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))) // extension
      -> decltype(_base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...))
  {
    return _base::template _apply_type_r<Ret>(::std::move(*this), FWD(f), FWD(args)...);
  }

  // Monadic operations. Bodies delegate to _expected_base static helpers, which perform sum-widening.
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

  // Convert to graded monad. There is no value to relocate here, so only the error's lift weighs.
  constexpr auto sum_error() const & noexcept(::std::is_nothrow_constructible_v<sum<error_type>, error_type const &>
                                              && ::std::is_nothrow_move_constructible_v<sum<error_type>>) // extension
      -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{::std::in_place};
    else
      return type{::fn::unexpect, sum<error_type>(this->error())};
  }
  constexpr auto sum_error() && noexcept(::std::is_nothrow_constructible_v<sum<error_type>, error_type>
                                         && ::std::is_nothrow_move_constructible_v<sum<error_type>>) // extension
      -> expected<value_type, sum<error_type>>
    requires(not some_sum<error_type>)
  {
    using type = expected<value_type, sum<error_type>>;
    if (this->has_value())
      return type{::std::in_place};
    else
      return type{::fn::unexpect, sum<error_type>(::std::move(*this).error())};
  }
  constexpr auto sum_error() & noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return *this;
  }
  constexpr auto sum_error() const & noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return *this;
  }
  constexpr auto sum_error() && noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto sum_error() const && noexcept -> decltype(auto)
    requires(some_sum<error_type>)
  {
    return ::std::move(*this);
  }

private:
  // Direct-non-list-initializes the error member from the result of a callable; used by the
  // monadic functions implemented in _expected_base.
  template <class Tag, class Fn, class... Args>
  constexpr explicit expected(::pfn::detail::_expected_from_invoke_t tag, Tag which, Fn &&fn, Args &&...args) //
      noexcept(::std::is_nothrow_constructible_v<_base, ::pfn::detail::_expected_from_invoke_t, Tag, Fn, Args...>)
      : _base(tag, which, FWD(fn), FWD(args)...)
  {
  }
};
// Lifts for sum transformation functions
[[nodiscard]] constexpr auto sum_value(some_expected_non_void auto &&src) noexcept(noexcept(FWD(src).sum_value()))
    -> decltype(auto)
{
  return FWD(src).sum_value();
}
[[nodiscard]] constexpr auto sum_error(some_expected auto &&src) noexcept(noexcept(FWD(src).sum_error()))
    -> decltype(auto)
{
  return FWD(src).sum_error();
}

namespace detail {
template <typename E> struct _expected_type {
  template <typename T> using type = ::fn::expected<T, E>;
};

// `error()` throws when the expected holds a value, but every arm below is reached only once
// `has_value()` has answered - so these ask what constructing the result promises, with the accessor
// spelled as a type rather than as a call which would drag its own throw in.
template <typename Type, typename Lh, typename Rh, typename... Vs>
constexpr inline bool _nothrow_join_expected
    = _nothrow_initializable<Type, ::std::in_place_t, Vs...>
      && _nothrow_initializable<Type, ::fn::unexpect_t, decltype(::std::declval<Lh>().error())>
      && _nothrow_initializable<Type, ::fn::unexpect_t, decltype(::std::declval<Rh>().error())>;

// Lifting an operand's error into a widened error type, through the sum<> guard of _nothrow_error_arm
// (the joins below assert the same unreachability with pfn::unreachable).
template <typename Src, typename Err>
constexpr inline bool _nothrow_error_lift
    = _nothrow_error_arm<typename ::std::remove_cvref_t<Src>::error_type, Err, decltype(::std::declval<Src>().error())>;

template <typename Type, typename Err, typename Lh, typename Rh, typename... Vs>
constexpr inline bool _nothrow_join_widened = _nothrow_initializable<Type, ::std::in_place_t, Vs...>
                                              && (_nothrow_error_lift<Lh, Err> && _nothrow_error_lift<Rh, Err>)
                                              && _nothrow_initializable<Type, ::fn::unexpect_t, Err>;

// A named type, not a lambda: `operator&` specifies itself in terms of what lifting the error
// promises, and a lambda can be named neither in a noexcept-specifier nor (before clang 17) in any
// unevaluated operand at all.
template <typename E> struct _expected_efn final {
  // Explicit return type: for a sum<> (never-erroring) operand the else branch is the only one
  // instantiated, and without this it would deduce void - poisoning _join's return-type deduction.
  [[nodiscard]] constexpr auto operator()(auto &&v) const noexcept(_nothrow_error_lift<decltype(v), unexpected<E>>)
      -> ::fn::unexpected<E>
  {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<decltype(v)>::error_type, sum<>>) {
      return ::fn::unexpected<E>(FWD(v).error());
    } else {
      ::pfn::unreachable(); // LCOV_EXCL_LINE
    }
  }
};
} // namespace detail

// When any of the sides is expected<void, ...>, we do not produce expected<pack<...>, ...>
// Instead just elide void and carry non-void (or elide both voids if that's what we get)
template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && (not some_expected_void<Rh>)
           && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                               typename ::std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_expected<
             expected<typename ::std::remove_cvref_t<Rh>::value_type, typename ::std::remove_cvref_t<Lh>::error_type>,
             Lh, Rh, decltype(FWD(rh).value())>)
{
  using error_type = ::std::remove_cvref_t<Lh>::error_type;
  using value_type = ::std::remove_cvref_t<Rh>::value_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, FWD(rh).value()};
  else if (not lh.has_value())
    return type{::fn::unexpect, FWD(lh).error()};
  else
    return type{::fn::unexpect, FWD(rh).error()};
}

template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && (not some_expected_void<Rh>)
           && (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                                    typename ::std::remove_cvref_t<Rh>::error_type>)
           && (some_sum<typename ::std::remove_cvref_t<Lh>::error_type>
               || some_sum<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_widened<
             expected<typename ::std::remove_cvref_t<Rh>::value_type,
                      sum_for<typename ::std::remove_cvref_t<Lh>::error_type,
                              typename ::std::remove_cvref_t<Rh>::error_type>>,
             sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>,
             Lh, Rh, decltype(FWD(rh).value())>)
{
  using new_error_type
      = sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  using value_type = ::std::remove_cvref_t<Rh>::value_type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, FWD(rh).value()};
  else if (not lh.has_value()) {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type, sum<>>)
      return type{::fn::unexpect, new_error_type{FWD(lh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  } else {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Rh>::error_type, sum<>>)
      return type{::fn::unexpect, new_error_type{FWD(rh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  }
}

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && some_expected_void<Rh>
          && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                              typename ::std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_expected<
             expected<typename ::std::remove_cvref_t<Lh>::value_type, typename ::std::remove_cvref_t<Lh>::error_type>,
             Lh, Rh, decltype(FWD(lh).value())>)
{
  using error_type = ::std::remove_cvref_t<Lh>::error_type;
  using value_type = ::std::remove_cvref_t<Lh>::value_type;
  using type = expected<value_type, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, FWD(lh).value()};
  else if (not lh.has_value())
    return type{::fn::unexpect, FWD(lh).error()};
  else
    return type{::fn::unexpect, FWD(rh).error()};
}

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && some_expected_void<Rh>
          && (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                                   typename ::std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename ::std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_widened<
             expected<typename ::std::remove_cvref_t<Lh>::value_type,
                      sum_for<typename ::std::remove_cvref_t<Lh>::error_type,
                              typename ::std::remove_cvref_t<Rh>::error_type>>,
             sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>,
             Lh, Rh, decltype(FWD(lh).value())>)
{
  using new_error_type
      = sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  using value_type = ::std::remove_cvref_t<Lh>::value_type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, FWD(lh).value()};
  else if (not lh.has_value()) {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type, sum<>>)
      return type{::fn::unexpect, new_error_type{FWD(lh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  } else {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Rh>::error_type, sum<>>)
      return type{::fn::unexpect, new_error_type{FWD(rh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  }
}

template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && some_expected_void<Rh>
           && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                               typename ::std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_expected<expected<void, typename ::std::remove_cvref_t<Lh>::error_type>, Lh, Rh>)
{
  using error_type = ::std::remove_cvref_t<Lh>::error_type;
  using type = expected<void, error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place};
  else if (not lh.has_value())
    return type{::fn::unexpect, FWD(lh).error()};
  else
    return type{::fn::unexpect, FWD(rh).error()};
}

template <typename Lh, typename Rh>
  requires some_expected_void<Lh> && some_expected_void<Rh>
           && (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                                    typename ::std::remove_cvref_t<Rh>::error_type>)
           && (some_sum<typename ::std::remove_cvref_t<Lh>::error_type>
               || some_sum<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_widened<
             expected<void, sum_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                    typename ::std::remove_cvref_t<Rh>::error_type>>,
             sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>,
             Lh, Rh>)
{
  using new_error_type
      = sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  using type = expected<void, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place};
  else if (not lh.has_value()) {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type, sum<>>)
      return type{::fn::unexpect, new_error_type{FWD(lh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  } else {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Rh>::error_type, sum<>>)
      return type{::fn::unexpect, new_error_type{FWD(rh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  }
}

// Overloads when both sides are non-void, producing either of
// expected<pack<...>, ...> or expected<sum<pack<...>, pack...>, ...>
template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && (not some_expected_void<Rh>)
          && ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                              typename ::std::remove_cvref_t<Rh>::error_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(noexcept(::fn::detail::_join<
                      detail::template _expected_type<typename ::std::remove_cvref_t<Lh>::error_type>::template type>(
        FWD(lh), FWD(rh), detail::_expected_efn<typename ::std::remove_cvref_t<Lh>::error_type>{})))
{
  using error_type = ::std::remove_cvref_t<Lh>::error_type;
  return ::fn::detail::_join<detail::template _expected_type<error_type>::template type>(
      FWD(lh), FWD(rh), detail::_expected_efn<error_type>{});
}

template <typename Lh, typename Rh>
  requires(not some_expected_void<Lh>) && (not some_expected_void<Rh>)
          && (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type,
                                   typename ::std::remove_cvref_t<Rh>::error_type>)
          && (some_sum<typename ::std::remove_cvref_t<Lh>::error_type>
              || some_sum<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(noexcept(
        ::fn::detail::_join<
            detail::template _expected_type<sum_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                                    typename ::std::remove_cvref_t<Rh>::error_type>>::template type>(
            FWD(lh), FWD(rh),
            detail::_expected_efn<sum_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                          typename ::std::remove_cvref_t<Rh>::error_type>>{})))
{
  using new_error_type
      = sum_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  return ::fn::detail::_join<detail::template _expected_type<new_error_type>::template type>(
      FWD(lh), FWD(rh), detail::_expected_efn<new_error_type>{});
}

} // namespace fn

#endif // INCLUDE_FN_EXPECTED
