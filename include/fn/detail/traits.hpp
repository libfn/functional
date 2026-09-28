// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_TRAITS
#define INCLUDE_FN_DETAIL_TRAITS

#include <fn/detail/fwd.hpp>
#include <libfn_version.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace fn::inline LIBFN_VERSION::detail {

template <typename T> constexpr bool _is_in_place_type = false;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> &> = true;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> const &> = true;
template <typename T>
concept _some_in_place_type = _is_in_place_type<T &>;

// Reject incomplete types before trait results or concept satisfaction can depend
// on whether the definition has been seen.
template <typename T> constexpr bool _complete_class() noexcept
{
  if constexpr (::std::is_class_v<T> || ::std::is_union_v<T>)
    return sizeof(T) > 0; // incomplete: include the header defining this type first
  else
    return true;
}

template <typename T>
concept _some_monadic_type = _some_expected<T> || _some_optional<T> || _some_just<T>;

template <typename Functor, typename V, typename... Args>
concept _monadic_invocable
    = _complete_class<Functor>() && _some_monadic_type<V> && ::std::invocable<typename Functor::apply, V, Args...>;

// The storage initializes an element as `T{args...}`, so a constraint on it must ask the same
// question: `is_constructible_v` spells parenthesized initialization, which for an aggregate
// performs no brace elision (`std::array<int, 3>` is not "constructible" from 3 ints) and permits
// narrowing where brace initialization rejects it. A reference element is not brace-initialized but
// bound, and `T{...}` for a reference `T` is not even a portable question to ask (gcc rejects it,
// clang accepts) - so that leg asks about the binding instead.
template <typename T, typename... Args>
concept _initializable                                                    //
    = (::std::is_reference_v<T> && ::std::is_constructible_v<T, Args...>) //
      || ((not ::std::is_reference_v<T>) && requires { T{::std::declval<Args>()...}; });

// Whether that same initialization can throw. Only instantiated for an `_initializable` T, and for
// the same reason it cannot be `is_nothrow_constructible_v`: the question is about `T{args...}`.
template <typename T, typename... Args>
struct _nothrow_init : ::std::bool_constant<noexcept(T{::std::declval<Args>()...})> {};
template <typename T, typename... Args>
  requires ::std::is_reference_v<T>
struct _nothrow_init<T, Args...> : ::std::bool_constant<::std::is_nothrow_constructible_v<T, Args...>> {};

template <typename T, typename... Args>
concept _nothrow_initializable = _initializable<T, Args...> && _nothrow_init<T, Args...>::value;

// Change any rvalue or empty value to prvalue, but leave lvalues unchanged.
// This is meant to find the type of data members which won't bind to rvalues.
template <typename T> extern T _as_value;

template <typename T> extern T _as_value<T &&>;
template <typename T>
  requires(::std::is_empty_v<T>)
extern T _as_value<T &>;
template <typename T>
  requires(!::std::is_empty_v<T>)
extern T &_as_value<T &>;

template <typename T> extern T const _as_value<T const &&>;
template <typename T>
  requires(::std::is_empty_v<T>)
extern T const _as_value<T const &>;
template <typename T>
  requires(!::std::is_empty_v<T>)
extern T const &_as_value<T const &>;

// Add const to second type, if first type is const
template <typename T, typename V> extern V _apply_const;
template <typename T, typename V> extern V const _apply_const<T const &, V>;
template <typename T, typename V> extern V const &_apply_const<T const &, V &>;
template <typename T, typename V> extern V const &&_apply_const<T const &, V &&>;

// Add lvalue reference to second type, if first type is lvalue reference
template <typename T, typename V> extern V _apply_lvalue;
template <typename T, typename V> extern V &_apply_lvalue<T &, V>;
template <typename T, typename V> extern V &_apply_lvalue<T &, V &&>;

} // namespace fn::inline LIBFN_VERSION::detail

namespace fn {
inline namespace LIBFN_VERSION {
template <typename T, typename V>
using apply_const_lvalue_t = decltype(detail::_apply_const<T &, decltype(detail::_apply_lvalue<T, V>)>);
} // namespace LIBFN_VERSION
} // namespace fn

#endif // INCLUDE_FN_DETAIL_TRAITS
