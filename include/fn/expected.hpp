// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_EXPECTED
#define INCLUDE_FN_EXPECTED

#include <libfn_version.hpp>
#include <pfn/expected.hpp>
#include <pfn/utility.hpp>

#include <fn/copack.hpp>
#include <fn/detail/traits.hpp>
#include <fn/fwd.hpp>
#include <fn/pack.hpp>

#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

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

// A copack<> side is unconstructible, so an expected carrying one can never hold it: every arm that
// relocates such a side - self's or a callback result's, value or error - is unreachable, and what
// cannot run cannot throw. Keyed on the relocated side's type; the sibling arms still weigh.
template <typename From, typename Type, typename... Args>
constexpr inline bool _nothrow_arm = _nothrow_initializable<Type, Args...>;
template <typename From, typename Type, typename... Args>
  requires empty_copack<From>
constexpr inline bool _nothrow_arm<From, Type, Args...> = true;

// Carrying the callback's value across into a widened result. An expected<void, ...> has no value to
// carry, and `declval<void>()` is not a thing to ask about; an empty-copack value can never exist to be
// carried, so that arm is unreachable (as in _nothrow_arm).
template <typename Type, typename Src>
constexpr inline bool _nothrow_carry_value
    = _nothrow_initializable<Type, ::std::in_place_t, decltype(::std::declval<Src>().value())>;
template <typename Type, typename Src>
  requires ::std::is_void_v<typename ::std::remove_cvref_t<Src>::value_type>
constexpr inline bool _nothrow_carry_value<Type, Src> = _nothrow_initializable<Type, ::std::in_place_t>;
template <typename Type, typename Src>
  requires empty_copack<typename ::std::remove_cvref_t<Src>::value_type>
constexpr inline bool _nothrow_carry_value<Type, Src> = true;

// `and_then` and `or_else` each have two arms - the callback's own expected is returned, or the two
// error (value) types are widened into a copack - and `if constexpr` picks between them. A
// noexcept-specifier is an ordinary constant expression and cannot pick: the untaken arm's spelling
// would have to be well-formed too. Hence a trait, whose constrained specializations mirror the
// body's arms. Both lead with `_is_some_expected`, so a callback returning something else leaves the
// unconstrained primary to answer, and the body's static_assert to diagnose - a specification must
// not pre-empt that with a hard error.
// The dispatch result of a graded bind: a plain side answers through _apply_result as always; a
// copack side goes through the graded join - select for a convergent set (today's behaviour and
// diagnostics verbatim), the joined expected for a heterogeneous all-expected one, absent for an
// invalid one - so the members and the traits below key on ONE answer and never instantiate the
// select assert for a shape the join owns.
template <typename E, typename Fn, typename... V> struct _and_then_dispatch : _apply_result<Fn, V...> {};
template <typename E, typename Fn, typename V>
  requires _some_copack<::std::remove_cvref_t<V>>
struct _and_then_dispatch<E, Fn, V> : _copack_apply_result<_joining_expected_tag<::fn::expected, E>, Fn, V> {};

template <typename T, typename Fn, typename ErrArg> struct _or_else_dispatch : _apply_result<Fn, ErrArg> {};
template <typename T, typename Fn, typename ErrArg>
  requires _some_copack<::std::remove_cvref_t<ErrArg>>
struct _or_else_dispatch<T, Fn, ErrArg> : _copack_apply_result<_joining_recovery_tag<::fn::expected, T>, Fn, ErrArg> {};

template <typename E, typename Fn, typename ErrArg, typename... ValArg> struct _nothrow_and_then : ::std::false_type {};

template <typename E, typename Fn, typename ErrArg, typename... ValArg>
  requires(not _is_hetero_join<_and_then_dispatch<E, Fn, ValArg...>>)
          && _is_some_expected<::std::remove_cvref_t<typename _and_then_dispatch<E, Fn, ValArg...>::type> &>
          && ::std::is_same_v<typename _expected_types<::std::remove_cvref_t<
                                  typename _and_then_dispatch<E, Fn, ValArg...>::type>>::error_type,
                              E>
struct _nothrow_and_then<E, Fn, ErrArg, ValArg...>
    : ::std::bool_constant<_is_nothrow_applicable<Fn, ValArg...>::value // the callback
                               && ::std::is_nothrow_constructible_v<
                                   ::std::remove_cvref_t<typename _and_then_dispatch<E, Fn, ValArg...>::type>,
                                   ::fn::unexpect_t, ErrArg>> {}; // lifting self's error

template <typename E, typename Fn, typename ErrArg, typename... ValArg>
  requires(not _is_hetero_join<_and_then_dispatch<E, Fn, ValArg...>>)
          && _is_some_expected<::std::remove_cvref_t<typename _and_then_dispatch<E, Fn, ValArg...>::type> &>
          && (not ::std::is_same_v<typename _expected_types<::std::remove_cvref_t<
                                       typename _and_then_dispatch<E, Fn, ValArg...>::type>>::error_type,
                                   E>)
struct _nothrow_and_then<E, Fn, ErrArg, ValArg...> {
  using type = ::std::remove_cvref_t<typename _and_then_dispatch<E, Fn, ValArg...>::type>;
  using new_type = ::fn::expected<typename type::value_type, copack_for<E, typename type::error_type>>;

  static constexpr bool value                        //
      = _is_nothrow_applicable<Fn, ValArg...>::value // the callback
        && _nothrow_carry_value<new_type, type>      // carrying its value across
        && _nothrow_arm<typename type::error_type, new_type, ::fn::unexpect_t,
                        decltype(::std::declval<type>().error())> // widening its error
        && _nothrow_arm<E, new_type, ::fn::unexpect_t, ErrArg>;   // widening self's error
};

// the heterogeneous join: each branch and its conversion into the announced result are weighed by
// the rts trait; widening self's error is the one other reachable construction, dead for copack<>
template <typename E, typename Fn, typename ErrArg, typename... ValArg>
  requires _is_hetero_join<_and_then_dispatch<E, Fn, ValArg...>>
struct _nothrow_and_then<E, Fn, ErrArg, ValArg...> {
  using type = typename _and_then_dispatch<E, Fn, ValArg...>::type;

  static constexpr bool value
      = _is_nothrow_rts_applicable<type, Fn, ValArg...>
        && (empty_copack<E> || ::std::is_nothrow_constructible_v<type, ::fn::unexpect_t, ErrArg>);
};

// or_else's arms, mirrored the same way. ValArg is the type of self's value as the body relocates it
// (spelled through apply_const_lvalue_t by the caller, since for a void T there is no value to name).
template <typename T, typename Fn, typename ErrArg, typename ValArg> struct _nothrow_or_else : ::std::false_type {};

template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires(not _is_hetero_join<_or_else_dispatch<T, Fn, ErrArg>>)
          && _is_some_expected<::std::remove_cvref_t<typename _or_else_dispatch<T, Fn, ErrArg>::type> &>
          && ::std::is_same_v<typename _expected_types<
                                  ::std::remove_cvref_t<typename _or_else_dispatch<T, Fn, ErrArg>::type>>::value_type,
                              T>
struct _nothrow_or_else<T, Fn, ErrArg, ValArg>
    : ::std::bool_constant<
          _is_nothrow_applicable<Fn, ErrArg>::value // the callback
          && (::std::is_void_v<T>
              || _nothrow_initializable<::std::remove_cvref_t<typename _or_else_dispatch<T, Fn, ErrArg>::type>,
                                        ::std::in_place_t, ValArg>)> {}; // carrying self's value

template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires(not _is_hetero_join<_or_else_dispatch<T, Fn, ErrArg>>)
          && _is_some_expected<::std::remove_cvref_t<typename _or_else_dispatch<T, Fn, ErrArg>::type> &>
          && (not ::std::is_same_v<typename _expected_types<::std::remove_cvref_t<
                                       typename _or_else_dispatch<T, Fn, ErrArg>::type>>::value_type,
                                   T>)
struct _nothrow_or_else<T, Fn, ErrArg, ValArg> {
  using type = ::std::remove_cvref_t<typename _or_else_dispatch<T, Fn, ErrArg>::type>;
  using new_type = ::fn::expected<copack_for<T, typename type::value_type>, typename type::error_type>;

  static constexpr bool value                                   //
      = _is_nothrow_applicable<Fn, ErrArg>::value               // the callback
        && _nothrow_arm<T, new_type, ::std::in_place_t, ValArg> // widening self's value
        && _nothrow_carry_value<new_type, type>                 // widening its value
        && _nothrow_initializable<new_type, ::fn::unexpect_t,
                                  decltype(::std::declval<type>().error())>; // carrying its error
};

// the heterogeneous join, mirrored: the branches and their conversions through the rts trait;
// carrying self's value into the announced result is the other reachable construction
template <typename T, typename Fn, typename ErrArg, typename ValArg>
  requires _is_hetero_join<_or_else_dispatch<T, Fn, ErrArg>>
struct _nothrow_or_else<T, Fn, ErrArg, ValArg> {
  using type = typename _or_else_dispatch<T, Fn, ErrArg>::type;

  static constexpr bool value
      = _is_nothrow_rts_applicable<type, Fn, ErrArg>
        && (::std::is_void_v<T> || empty_copack<T> || _nothrow_initializable<type, ::std::in_place_t, ValArg>);
};

// Storage layer for ::fn::expected. Inherits the standard-conformant base from
// pfn, then hides the four monadic static helpers with copack-widening variants
// that materialise their result via `expected_policy::template type<U, G>`.
// The transform/transform_error helpers hand pfn's _expected_from_invoke constructors a
// zero-argument thunk, so the result's member is direct-non-list-initialized from fn's own
// _apply (or copack::transform) result: no extra move, and immovable result types work.
// The statics carry the same extension noexcept as pfn's, computed through fn's own machinery: the
// callback of a copack/pack dispatch is invoked through `_apply`, not called directly, so it is
// `_is_nothrow_applicable` - not the std trait, which is false for a callable that is not directly
// applicable on a copack or a pack - that answers for it, and the widening arms are weighed by the
// traits above.
template <typename T, typename E> struct _expected_base : ::pfn::detail::_expected_base<T, E, expected_policy> {
  using _pfn_base = ::pfn::detail::_expected_base<T, E, expected_policy>;
  using _pfn_base::_pfn_base;

  // and_then, non-void value type
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_nothrow_and_then<E, Fn, decltype(_pfn_base::_error(FWD(self))),
                                               decltype(_pfn_base::_value(FWD(self)))>::value) // extension
    requires(not ::std::is_void_v<T>) && (not empty_copack<T>)
            && ::fn::detail::_bind_applicable<Fn, decltype(_pfn_base::_value(FWD(self)))>
            && ::std::is_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))> && requires {
                 typename ::fn::detail::_and_then_dispatch<E, Fn, decltype(_pfn_base::_value(FWD(self)))>::type;
               }
  {
    using dispatch = ::fn::detail::_and_then_dispatch<E, Fn, decltype(_pfn_base::_value(FWD(self)))>;
    using type = typename dispatch::type;
    if constexpr (::fn::detail::_is_hetero_join<dispatch>) {
      // heterogeneous expected branches: the join announced `type`, every branch converts into it
      // as it returns, and the error path widens self's grade the same way
      if (self.has_value())
        return ::fn::detail::_tagged_join_apply<::fn::detail::_joining_expected_tag<::fn::expected, E>>(
            _pfn_base::_value(FWD(self)), FWD(fn));
      else {
        if constexpr (not empty_copack<E>)
          return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
        else
          ::pfn::unreachable(); // LCOV_EXCL_LINE
      }
    } else {
      static_assert(_is_some_expected<type &>);
      static_assert(::std::is_same_v<typename type::error_type, E> || some_copack<E>
                    || ::std::is_same_v<typename type::error_type, ::fn::copack<E>>);
      if constexpr (::std::is_same_v<typename type::error_type, E>) {
        if (self.has_value())
          return ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)));
        else
          return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
      } else {
        using new_error_type = copack_for<E, typename type::error_type>;
        using new_type = ::fn::expected<typename type::value_type, new_error_type>;
        if (self.has_value()) {
          auto t = ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)));
          if (t.has_value())
            if constexpr (not ::std::is_void_v<typename new_type::value_type>)
              return new_type{::std::in_place, ::std::move(t).value()};
            else
              return new_type{::std::in_place};
          else {
            if constexpr (not empty_copack<typename type::error_type>)
              return new_type{::fn::unexpect, ::std::move(t).error()};
            else
              ::pfn::unreachable(); // LCOV_EXCL_LINE
          }
        } else {
          if constexpr (not empty_copack<E>)
            return new_type(::fn::unexpect, _pfn_base::_error(FWD(self)));
          else
            ::pfn::unreachable(); // LCOV_EXCL_LINE
        }
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
    static_assert(::std::is_same_v<typename type::error_type, E> || some_copack<E>
                  || ::std::is_same_v<typename type::error_type, ::fn::copack<E>>);
    if constexpr (::std::is_same_v<typename type::error_type, E>) {
      if (self.has_value())
        return ::fn::detail::_apply(FWD(fn));
      else
        return type(::fn::unexpect, _pfn_base::_error(FWD(self)));
    } else {
      using new_error_type = copack_for<E, typename type::error_type>;
      using new_type = ::fn::expected<typename type::value_type, new_error_type>;
      if (self.has_value()) {
        auto t = ::fn::detail::_apply(FWD(fn));
        if (t.has_value())
          if constexpr (not ::std::is_void_v<typename new_type::value_type>)
            return new_type{::std::in_place, ::std::move(t).value()};
          else
            return new_type{::std::in_place};
        else {
          if constexpr (not empty_copack<typename type::error_type>)
            return new_type{::fn::unexpect, ::std::move(t).error()};
          else
            ::pfn::unreachable(); // LCOV_EXCL_LINE
        }
      } else {
        if constexpr (not empty_copack<E>)
          return new_type(::fn::unexpect, _pfn_base::_error(FWD(self)));
        else
          ::pfn::unreachable(); // LCOV_EXCL_LINE
      }
    }
  }

  // and_then, value type is the empty copack: a value can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, and the
  // result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _and_then(Self &&self, Fn &&)                         //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires empty_copack<T> && ::std::is_constructible_v<::fn::expected<T, E>, Self>
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
    requires(not empty_copack<E>) && ::fn::detail::_bind_applicable<Fn, decltype(_pfn_base::_error(FWD(self)))>
            && (::std::is_void_v<T> || ::std::is_constructible_v<T, decltype(_pfn_base::_value(FWD(self)))>)
            && requires {
                 typename ::fn::detail::_or_else_dispatch<T, Fn, decltype(_pfn_base::_error(FWD(self)))>::type;
               }
  {
    using dispatch = ::fn::detail::_or_else_dispatch<T, Fn, decltype(_pfn_base::_error(FWD(self)))>;
    using type = typename dispatch::type;
    if constexpr (::fn::detail::_is_hetero_join<dispatch>) {
      // heterogeneous expected branches: the join announced `type`; self's value widens into it on
      // the value path, and each branch converts into it as it returns on the error path
      if (self.has_value()) {
        if constexpr (::std::is_void_v<T>)
          return type{::std::in_place};
        else if constexpr (not empty_copack<T>)
          return type{::std::in_place, _pfn_base::_value(FWD(self))};
        else
          ::pfn::unreachable(); // LCOV_EXCL_LINE
      } else
        return ::fn::detail::_tagged_join_apply<::fn::detail::_joining_recovery_tag<::fn::expected, T>>(
            _pfn_base::_error(FWD(self)), FWD(fn));
    } else {
      static_assert(_is_some_expected<type &>);
      static_assert(::std::is_same_v<typename type::value_type, T> || some_copack<T>
                    || ::std::is_same_v<typename type::value_type, ::fn::copack<T>>);
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
        using new_value_type = copack_for<T, typename type::value_type>;
        using new_type = ::fn::expected<new_value_type, typename type::error_type>;
        if (self.has_value()) {
          if constexpr (not empty_copack<T>)
            return new_type{::std::in_place, _pfn_base::_value(FWD(self))};
          else
            ::pfn::unreachable(); // LCOV_EXCL_LINE
        } else {
          auto t = ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)));
          if (t.has_value()) {
            if constexpr (not empty_copack<typename type::value_type>)
              return new_type{::std::in_place, ::std::move(t).value()};
            else
              ::pfn::unreachable(); // LCOV_EXCL_LINE
          } else
            return new_type{::fn::unexpect, ::std::move(t).error()};
        }
      }
    }
  }

  // or_else, error type is the empty copack: an error can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, and the
  // result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _or_else(Self &&self, Fn &&)                          //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires empty_copack<E> && ::std::is_constructible_v<::fn::expected<T, E>, Self>
  {
    return FWD(self);
  }

  // transform, non-void value type, not a copack. In the noexcept specs of the transform and
  // transform_error overloads, only the apply and copying the untouched side can throw: the
  // new value/error is direct-non-list-initialized from the thunk's result (guaranteed elision).
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_value(FWD(self)))>::value
               && ::std::is_nothrow_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>) // extension
    requires(not ::std::is_void_v<T>) && (not some_copack<T>)
            && ::fn::detail::_is_applicable_if<not some_copack<T>, Fn, decltype(_pfn_base::_value(FWD(self)))>::value
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

  // transform, value type is a copack (delegates to copack::transform). The callback is constrained here,
  // in the immediate context, for the reason given on optional's copack-case _transform.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&fn) //
      noexcept(noexcept(_pfn_base::_value(FWD(self)).transform(FWD(fn)))
               && ::std::is_nothrow_constructible_v<E, decltype(_pfn_base::_error(FWD(self)))>) // extension
    requires some_copack<T> && (not empty_copack<T>)
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

  // transform, value type is the empty copack: a value can never be constructed, so the callback can
  // never be presented one - it is left alone, not invoked and not even instantiated, the mapping
  // is the identity and the result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _transform(Self &&self, Fn &&)                        //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires empty_copack<T> && ::std::is_constructible_v<::fn::expected<T, E>, Self>
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

  // transform_error, error type is not a copack (the value-copy conjunct is spelled via
  // apply_const_lvalue_t for the same reason as _or_else's above)
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_error(FWD(self)))>::value
               && (::std::is_void_v<T>
                   || ::std::is_nothrow_constructible_v<
                       T, ::fn::apply_const_lvalue_t<Self, typename _pfn_base::_value_t &&>>)) // extension
    requires(not some_copack<E>)
            && ::fn::detail::_is_applicable_if<not some_copack<E>, Fn, decltype(_pfn_base::_error(FWD(self)))>::value
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

  // transform_error, error type is a copack (delegates to copack::transform). The callback is constrained
  // here, in the immediate context, for the reason given on optional's copack-case _transform.
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&fn) //
      noexcept(noexcept(_pfn_base::_error(FWD(self)).transform(FWD(fn)))
               && (::std::is_void_v<T>
                   || ::std::is_nothrow_constructible_v<
                       T, ::fn::apply_const_lvalue_t<Self, typename _pfn_base::_value_t &&>>)) // extension
    requires some_copack<E> && (not empty_copack<E>)
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
  // side's value through fn's own _apply (a pack or tuple-like payload by elements, a copack by
  // dispatch); a void value arm is invoked without a value. Over an empty-copack side this overload
  // set needs no gate: copack<> has no apply, so _is_applicable and _apply_tagged answer false for
  // every Fn and the general overloads drop out.
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

  // apply, error type is the empty copack: the error row is uninhabited, so the value arm alone is
  // exhaustive and dispatch needs no branch; nothing names the error row, so an arm set carrying
  // an arm for it never instantiates it.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(
          ::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value) // extension
      -> decltype(auto)
    requires(not ::std::is_void_v<T>) && empty_copack<E>
            && ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value
  {
    return ::fn::detail::_apply(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
  }

  // apply, void value type and empty copack error
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&, Fn &&fn, Args &&...args)         //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, Args...>::value) // extension
      -> decltype(auto)
    requires ::std::is_void_v<T> && empty_copack<E> && ::fn::detail::_is_applicable<Fn, Args...>::value
  {
    return ::fn::detail::_apply(FWD(fn), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, decltype(_pfn_base::_value(FWD(self))),
                                                      Args...>::value) // extension
      -> Ret
    requires(not ::std::is_void_v<T>) && empty_copack<E>
            && ::fn::detail::_is_applicable_r<Ret, Fn, decltype(_pfn_base::_value(FWD(self))), Args...>::value
  {
    return ::fn::detail::_apply_r<Ret>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
  }

  // apply_r, void value type and empty copack error
  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&, Fn &&fn, Args &&...args)              //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, Args...>::value) // extension
      -> Ret
    requires ::std::is_void_v<T> && empty_copack<E> && ::fn::detail::_is_applicable_r<Ret, Fn, Args...>::value
  {
    return ::fn::detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
  }

  // apply_type, error type is the empty copack: the value arm alone is exhaustive
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)),
                                                                       FWD(args)...))) // extension
      -> decltype(auto)
    requires(not ::std::is_void_v<T>) && empty_copack<E> && requires {
      ::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    }
  {
    return ::fn::detail::_apply_tagged<::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
  }

  // apply_type, void value type and empty copack error
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&, Fn &&fn, Args &&...args)                          //
      noexcept(::fn::detail::_is_nothrow_applicable<Fn, ::std::in_place_t, Args &&...>::value) // extension
      -> decltype(auto)
    requires ::std::is_void_v<T> && empty_copack<E>
             && ::fn::detail::_is_applicable<Fn, ::std::in_place_t, Args &&...>::value
  {
    return ::fn::detail::_apply(FWD(fn), ::std::in_place_t{}, FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)),
                                                                              FWD(args)...))) // extension
      -> Ret
    requires(not ::std::is_void_v<T>) && empty_copack<E> && requires {
      ::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
    }
  {
    return ::fn::detail::_apply_tagged_r<Ret, ::std::in_place_t>(FWD(fn), _pfn_base::_value(FWD(self)), FWD(args)...);
  }

  // apply_type_r, void value type and empty copack error
  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&, Fn &&fn, Args &&...args)                               //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, ::std::in_place_t, Args &&...>::value) // extension
      -> Ret
    requires ::std::is_void_v<T> && empty_copack<E>
             && ::fn::detail::_is_applicable_r<Ret, Fn, ::std::in_place_t, Args &&...>::value
  {
    return ::fn::detail::_apply_r<Ret>(FWD(fn), ::std::in_place_t{}, FWD(args)...);
  }

  // apply, value type is the empty copack: the value row is uninhabited, so the error arm alone is
  // exhaustive and dispatch needs no branch.
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(
          ::fn::detail::_is_nothrow_applicable<Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value) // extension
      -> decltype(auto)
    requires empty_copack<T> && ::fn::detail::_is_applicable<Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value
  {
    return ::fn::detail::_apply(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(::fn::detail::_is_nothrow_applicable_r<Ret, Fn, decltype(_pfn_base::_error(FWD(self))),
                                                      Args...>::value) // extension
      -> Ret
    requires empty_copack<T>
             && ::fn::detail::_is_applicable_r<Ret, Fn, decltype(_pfn_base::_error(FWD(self))), Args...>::value
  {
    return ::fn::detail::_apply_r<Ret>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // apply_type, value type is the empty copack: the error arm alone is exhaustive
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                      FWD(args)...))) // extension
      -> decltype(auto)
    requires empty_copack<T> && requires {
      ::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
    }
  {
    return ::fn::detail::_apply_tagged<::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  template <typename Ret, typename Self, typename Fn, typename... Args>
  static constexpr auto _apply_type_r(Self &&self, Fn &&fn, Args &&...args) //
      noexcept(noexcept(::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)),
                                                                             FWD(args)...))) // extension
      -> Ret
    requires empty_copack<T> && requires {
      ::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
    }
  {
    return ::fn::detail::_apply_tagged_r<Ret, ::fn::unexpect_t>(FWD(fn), _pfn_base::_error(FWD(self)), FWD(args)...);
  }

  // transform_error, error type is the empty copack: an error can never be constructed, so the
  // callback can never be presented one - it is left alone, not invoked and not even instantiated,
  // the mapping is the identity and the result is *this unchanged.
  template <typename Self, typename Fn>
  static constexpr auto _transform_error(Self &&self, Fn &&)                  //
      noexcept(::std::is_nothrow_constructible_v<::fn::expected<T, E>, Self>) // extension
      -> ::fn::expected<T, E>
    requires empty_copack<E> && ::std::is_constructible_v<::fn::expected<T, E>, Self>
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

  // Elimination over both states, mirroring copack's apply family: each arm takes its side's value
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

  // Monadic operations. Bodies delegate to _expected_base static helpers, which perform copack-widening.
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

  // Convert to graded monad. A lifting overload wraps one side in a copack and relocates the other
  // untouched, so it weighs both; the ones whose side already is a copack only return *this.
  constexpr auto
  copack_error() const & noexcept(::std::is_nothrow_constructible_v<value_type, value_type const &>
                                  && ::std::is_nothrow_constructible_v<copack<error_type>, error_type const &>
                                  && ::std::is_nothrow_move_constructible_v<copack<error_type>>) // extension
      -> expected<value_type, copack<error_type>>
    requires(not some_copack<error_type>)
  {
    using type = expected<value_type, copack<error_type>>;
    if (this->has_value())
      return type{::std::in_place, this->value()};
    else
      return type{::fn::unexpect, copack<error_type>(this->error())};
  }
  constexpr auto copack_error() && noexcept(::std::is_nothrow_constructible_v<value_type, value_type>
                                            && ::std::is_nothrow_constructible_v<copack<error_type>, error_type>
                                            && ::std::is_nothrow_move_constructible_v<copack<error_type>>) // extension
      -> expected<value_type, copack<error_type>>
    requires(not some_copack<error_type>)
  {
    using type = expected<value_type, copack<error_type>>;
    if (this->has_value())
      return type{::std::in_place, ::std::move(*this).value()};
    else
      return type{::fn::unexpect, copack<error_type>(::std::move(*this).error())};
  }
  constexpr auto copack_error() & noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return *this;
  }
  constexpr auto copack_error() const & noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return *this;
  }
  constexpr auto copack_error() && noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto copack_error() const && noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return ::std::move(*this);
  }

  constexpr auto
  copack_value() const & noexcept(::std::is_nothrow_constructible_v<copack<value_type>, value_type const &>
                                  && ::std::is_nothrow_move_constructible_v<copack<value_type>>
                                  && ::std::is_nothrow_constructible_v<error_type, error_type const &>) // extension
      -> expected<copack<value_type>, error_type>
    requires(not some_copack<value_type>)
  {
    using type = expected<copack<value_type>, error_type>;
    if (this->has_value())
      return type{::std::in_place, copack<value_type>(this->value())};
    else
      return type{::fn::unexpect, this->error()};
  }
  constexpr auto copack_value() && noexcept(::std::is_nothrow_constructible_v<copack<value_type>, value_type>
                                            && ::std::is_nothrow_move_constructible_v<copack<value_type>>
                                            && ::std::is_nothrow_constructible_v<error_type, error_type>) // extension
      -> expected<copack<value_type>, error_type>
    requires(not some_copack<value_type>)
  {
    using type = expected<copack<value_type>, error_type>;
    if (this->has_value())
      return type{::std::in_place, copack<value_type>(::std::move(*this).value())};
    else
      return type{::fn::unexpect, ::std::move(*this).error()};
  }
  constexpr auto copack_value() & noexcept -> decltype(auto)
    requires(some_copack<value_type>)
  {
    return *this;
  }
  constexpr auto copack_value() const & noexcept -> decltype(auto)
    requires(some_copack<value_type>)
  {
    return *this;
  }
  constexpr auto copack_value() && noexcept -> decltype(auto)
    requires(some_copack<value_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto copack_value() const && noexcept -> decltype(auto)
    requires(some_copack<value_type>)
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

  // Elimination over both states, mirroring copack's apply family: the value arm takes no value
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

  // Monadic operations. Bodies delegate to _expected_base static helpers, which perform copack-widening.
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
  constexpr auto
  copack_error() const & noexcept(::std::is_nothrow_constructible_v<copack<error_type>, error_type const &>
                                  && ::std::is_nothrow_move_constructible_v<copack<error_type>>) // extension
      -> expected<value_type, copack<error_type>>
    requires(not some_copack<error_type>)
  {
    using type = expected<value_type, copack<error_type>>;
    if (this->has_value())
      return type{::std::in_place};
    else
      return type{::fn::unexpect, copack<error_type>(this->error())};
  }
  constexpr auto copack_error() && noexcept(::std::is_nothrow_constructible_v<copack<error_type>, error_type>
                                            && ::std::is_nothrow_move_constructible_v<copack<error_type>>) // extension
      -> expected<value_type, copack<error_type>>
    requires(not some_copack<error_type>)
  {
    using type = expected<value_type, copack<error_type>>;
    if (this->has_value())
      return type{::std::in_place};
    else
      return type{::fn::unexpect, copack<error_type>(::std::move(*this).error())};
  }
  constexpr auto copack_error() & noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return *this;
  }
  constexpr auto copack_error() const & noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return *this;
  }
  constexpr auto copack_error() && noexcept -> decltype(auto)
    requires(some_copack<error_type>)
  {
    return ::std::move(*this);
  }
  constexpr auto copack_error() const && noexcept -> decltype(auto)
    requires(some_copack<error_type>)
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
// Lifts for copack transformation functions
[[nodiscard]] constexpr auto copack_value(some_expected_non_void auto &&src) noexcept(noexcept(FWD(src).copack_value()))
    -> decltype(auto)
{
  return FWD(src).copack_value();
}
[[nodiscard]] constexpr auto copack_error(some_expected auto &&src) noexcept(noexcept(FWD(src).copack_error()))
    -> decltype(auto)
{
  return FWD(src).copack_error();
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

// Lifting an operand's error into a widened error type, through the copack<> guard of _nothrow_arm
// (the joins below assert the same unreachability with pfn::unreachable).
template <typename Src, typename Err>
constexpr inline bool _nothrow_error_lift
    = _nothrow_arm<typename ::std::remove_cvref_t<Src>::error_type, Err, decltype(::std::declval<Src>().error())>;

template <typename Type, typename Err, typename Lh, typename Rh, typename... Vs>
constexpr inline bool _nothrow_join_widened = _nothrow_initializable<Type, ::std::in_place_t, Vs...>
                                              && (_nothrow_error_lift<Lh, Err> && _nothrow_error_lift<Rh, Err>)
                                              && _nothrow_initializable<Type, ::fn::unexpect_t, Err>;

// A named type, not a lambda: `operator&` specifies itself in terms of what lifting the error
// promises, and a lambda can be named neither in a noexcept-specifier nor (before clang 17) in any
// unevaluated operand at all.
template <typename E> struct _expected_efn final {
  // Explicit return type: for a copack<> (never-erroring) operand the else branch is the only one
  // instantiated, and without this it would deduce void - poisoning _join's return-type deduction.
  [[nodiscard]] constexpr auto operator()(auto &&v) const noexcept(_nothrow_error_lift<decltype(v), unexpected<E>>)
      -> ::fn::unexpected<E>
  {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<decltype(v)>::error_type, copack<>>) {
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
           && (some_copack<typename ::std::remove_cvref_t<Lh>::error_type>
               || some_copack<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_widened<
             expected<typename ::std::remove_cvref_t<Rh>::value_type,
                      copack_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                 typename ::std::remove_cvref_t<Rh>::error_type>>,
             copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>,
             Lh, Rh, decltype(FWD(rh).value())>)
{
  using new_error_type
      = copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  using value_type = ::std::remove_cvref_t<Rh>::value_type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, FWD(rh).value()};
  else if (not lh.has_value()) {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type, copack<>>)
      return type{::fn::unexpect, new_error_type{FWD(lh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  } else {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Rh>::error_type, copack<>>)
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
          && (some_copack<typename ::std::remove_cvref_t<Lh>::error_type>
              || some_copack<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_widened<
             expected<typename ::std::remove_cvref_t<Lh>::value_type,
                      copack_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                 typename ::std::remove_cvref_t<Rh>::error_type>>,
             copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>,
             Lh, Rh, decltype(FWD(lh).value())>)
{
  using new_error_type
      = copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  using value_type = ::std::remove_cvref_t<Lh>::value_type;
  using type = expected<value_type, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place, FWD(lh).value()};
  else if (not lh.has_value()) {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type, copack<>>)
      return type{::fn::unexpect, new_error_type{FWD(lh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  } else {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Rh>::error_type, copack<>>)
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
           && (some_copack<typename ::std::remove_cvref_t<Lh>::error_type>
               || some_copack<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_join_widened<
             expected<void, copack_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                       typename ::std::remove_cvref_t<Rh>::error_type>>,
             copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>,
             Lh, Rh>)
{
  using new_error_type
      = copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  using type = expected<void, new_error_type>;
  if (lh.has_value() && rh.has_value())
    return type{::std::in_place};
  else if (not lh.has_value()) {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Lh>::error_type, copack<>>)
      return type{::fn::unexpect, new_error_type{FWD(lh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  } else {
    if constexpr (not ::std::is_same_v<typename ::std::remove_cvref_t<Rh>::error_type, copack<>>)
      return type{::fn::unexpect, new_error_type{FWD(rh).error()}};
    else
      ::pfn::unreachable(); // LCOV_EXCL_LINE
  }
}

// Overloads when both sides are non-void, producing either of
// expected<pack<...>, ...> or expected<copack<pack<...>, pack...>, ...>
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
          && (some_copack<typename ::std::remove_cvref_t<Lh>::error_type>
              || some_copack<typename ::std::remove_cvref_t<Rh>::error_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(noexcept(
        ::fn::detail::_join<
            detail::template _expected_type<copack_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                                       typename ::std::remove_cvref_t<Rh>::error_type>>::template type>(
            FWD(lh), FWD(rh),
            detail::_expected_efn<copack_for<typename ::std::remove_cvref_t<Lh>::error_type,
                                             typename ::std::remove_cvref_t<Rh>::error_type>>{})))
{
  using new_error_type
      = copack_for<typename ::std::remove_cvref_t<Lh>::error_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  return ::fn::detail::_join<detail::template _expected_type<new_error_type>::template type>(
      FWD(lh), FWD(rh), detail::_expected_efn<new_error_type>{});
}

namespace detail {
// The cluster conjunction's specification: the cluster operand always contributes its value, so
// only the expected operand's channels weigh - its error relocating unchanged into the result.
template <bool Uninhabited, typename Type, typename Lh, typename Rh, typename Err> struct _nothrow_amp_cluster {
  static constexpr bool value = _nothrow_initializable<Type, ::fn::unexpect_t, Err>;
};
template <typename Type, typename Lh, typename Rh, typename Err> struct _nothrow_amp_cluster<false, Type, Lh, Rh, Err> {
  static constexpr bool value
      = noexcept(::fn::detail::_fold_detail::fold<typename ::std::remove_cvref_t<Lh>::value_type,
                                                  typename ::std::remove_cvref_t<Rh>::value_type>(
            ::std::declval<::fn::detail::_value_of_t<Lh>>(), ::std::declval<::fn::detail::_value_of_t<Rh>>()))
        && _nothrow_initializable<Type, ::std::in_place_t, ::fn::detail::_joined_t<Lh, Rh>>
        && _nothrow_initializable<Type, ::fn::unexpect_t, Err>;
};
} // namespace detail

// The identity cluster in the conjunction: a just or choice operand always contributes its value
// to the product and adds no term to the error sum - the expected operand's error passes through
// unchanged, plain or graded, and its state alone decides. just<void> is the product's unit and
// elides.
template <typename Lh, some_expected Rh>
  requires(::fn::detail::_some_just<Lh> || ::fn::detail::_some_choice<Lh>)
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>) && (not some_expected_void<Rh>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_amp_cluster<
             ::fn::detail::_uninhabited_join<Lh, Rh>,
             expected<::fn::detail::_joined_t<Lh, Rh>, typename ::std::remove_cvref_t<Rh>::error_type>, Lh, Rh,
             decltype(FWD(rh).error())>::value)
{
  using type = expected<::fn::detail::_joined_t<Lh, Rh>, typename ::std::remove_cvref_t<Rh>::error_type>;
  if constexpr (::fn::detail::_uninhabited_join<Lh, Rh>) {
    return type{::fn::unexpect, FWD(rh).error()};
  } else {
    using VL = ::std::remove_cvref_t<Lh>::value_type;
    using VR = ::std::remove_cvref_t<Rh>::value_type;
    if (rh.has_value())
      return type{::std::in_place, ::fn::detail::_fold_detail::fold<VL, VR>(FWD(lh).value(), FWD(rh).value())};
    return type{::fn::unexpect, FWD(rh).error()};
  }
}

template <some_expected Lh, typename Rh>
  requires(::fn::detail::_some_just<Rh> || ::fn::detail::_some_choice<Rh>)
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>) && (not some_expected_void<Lh>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(detail::_nothrow_amp_cluster<
             ::fn::detail::_uninhabited_join<Lh, Rh>,
             expected<::fn::detail::_joined_t<Lh, Rh>, typename ::std::remove_cvref_t<Lh>::error_type>, Lh, Rh,
             decltype(FWD(lh).error())>::value)
{
  using type = expected<::fn::detail::_joined_t<Lh, Rh>, typename ::std::remove_cvref_t<Lh>::error_type>;
  if constexpr (::fn::detail::_uninhabited_join<Lh, Rh>) {
    return type{::fn::unexpect, FWD(lh).error()};
  } else {
    using VL = ::std::remove_cvref_t<Lh>::value_type;
    using VR = ::std::remove_cvref_t<Rh>::value_type;
    if (lh.has_value())
      return type{::std::in_place, ::fn::detail::_fold_detail::fold<VL, VR>(FWD(lh).value(), FWD(rh).value())};
    return type{::fn::unexpect, FWD(lh).error()};
  }
}

template <typename Lh, some_expected_void Rh>
  requires(::fn::detail::_some_just<Lh> || ::fn::detail::_some_choice<Lh>)
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(
        ::fn::detail::_nothrow_initializable<
            expected<typename ::std::remove_cvref_t<Lh>::value_type, typename ::std::remove_cvref_t<Rh>::error_type>,
            ::std::in_place_t, decltype(FWD(lh).value())>
        && ::fn::detail::_nothrow_initializable<
            expected<typename ::std::remove_cvref_t<Lh>::value_type, typename ::std::remove_cvref_t<Rh>::error_type>,
            ::fn::unexpect_t, decltype(FWD(rh).error())>)
        -> expected<typename ::std::remove_cvref_t<Lh>::value_type, typename ::std::remove_cvref_t<Rh>::error_type>
{
  using type = expected<typename ::std::remove_cvref_t<Lh>::value_type, typename ::std::remove_cvref_t<Rh>::error_type>;
  if (rh.has_value())
    return type{::std::in_place, FWD(lh).value()};
  return type{::fn::unexpect, FWD(rh).error()};
}

template <some_expected_void Lh, typename Rh>
  requires(::fn::detail::_some_just<Rh> || ::fn::detail::_some_choice<Rh>)
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>)
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&rh) //
    noexcept(
        ::fn::detail::_nothrow_initializable<
            expected<typename ::std::remove_cvref_t<Rh>::value_type, typename ::std::remove_cvref_t<Lh>::error_type>,
            ::std::in_place_t, decltype(FWD(rh).value())>
        && ::fn::detail::_nothrow_initializable<
            expected<typename ::std::remove_cvref_t<Rh>::value_type, typename ::std::remove_cvref_t<Lh>::error_type>,
            ::fn::unexpect_t, decltype(FWD(lh).error())>)
        -> expected<typename ::std::remove_cvref_t<Rh>::value_type, typename ::std::remove_cvref_t<Lh>::error_type>
{
  using type = expected<typename ::std::remove_cvref_t<Rh>::value_type, typename ::std::remove_cvref_t<Lh>::error_type>;
  if (lh.has_value())
    return type{::std::in_place, FWD(rh).value()};
  return type{::fn::unexpect, FWD(lh).error()};
}

template <typename Lh, some_expected Rh>
  requires ::fn::detail::_some_just<Lh> && ::std::is_void_v<typename ::std::remove_cvref_t<Lh>::value_type>
[[nodiscard]] constexpr auto operator&(Lh &&, Rh &&rh) //
    noexcept(::fn::detail::_nothrow_initializable<::std::remove_cvref_t<Rh>, Rh>) -> ::std::remove_cvref_t<Rh>
{
  return ::std::remove_cvref_t<Rh>{FWD(rh)};
}

template <some_expected Lh, typename Rh>
  requires ::fn::detail::_some_just<Rh> && ::std::is_void_v<typename ::std::remove_cvref_t<Rh>::value_type>
[[nodiscard]] constexpr auto operator&(Lh &&lh, Rh &&) //
    noexcept(::fn::detail::_nothrow_initializable<::std::remove_cvref_t<Lh>, Lh>) -> ::std::remove_cvref_t<Lh>
{
  return ::std::remove_cvref_t<Lh>{FWD(lh)};
}

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_EXPECTED
