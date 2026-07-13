// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_TRAITS
#define INCLUDE_FN_DETAIL_TRAITS

#include <type_traits>
#include <utility>

namespace fn::detail {

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

} // namespace fn::detail

namespace fn {
template <typename T, typename V>
using apply_const_lvalue_t = decltype(detail::_apply_const<T &, decltype(detail::_apply_lvalue<T, V>)>);
}

#endif // INCLUDE_FN_DETAIL_TRAITS
