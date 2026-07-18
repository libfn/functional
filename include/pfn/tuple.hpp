// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_PFN_TUPLE
#define INCLUDE_PFN_TUPLE

#include <array>
#include <complex>
#include <cstddef>
#include <functional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pfn {
namespace detail {

// [tuple.like] is enumerative - a specialization of array, complex, pair, tuple or
// ranges::subrange - not the tuple protocol: a user type modelling the protocol does not qualify.
template <typename T> constexpr bool _is_tuple_like_specialization = false;
template <typename T, ::std::size_t N> constexpr bool _is_tuple_like_specialization<::std::array<T, N>> = true;
template <typename T> constexpr bool _is_tuple_like_specialization<::std::complex<T>> = true;
template <typename T, typename U> constexpr bool _is_tuple_like_specialization<::std::pair<T, U>> = true;
template <typename... Ts> constexpr bool _is_tuple_like_specialization<::std::tuple<Ts...>> = true;
template <typename It, typename S, ::std::ranges::subrange_kind K>
constexpr bool _is_tuple_like_specialization<::std::ranges::subrange<It, S, K>> = true;

template <typename Tuple>
concept _tuple_like = _is_tuple_like_specialization<::std::remove_cvref_t<Tuple>>;

// `complex` is enumerated tuple-like, but its tuple protocol is C++26 (P2819): where the
// underlying standard library does not provide it, the traits below answer false rather than
// error, and open up when it appears.
template <typename Tuple>
constexpr bool _tuple_sized = requires { ::std::tuple_size<::std::remove_reference_t<Tuple>>::value; };

template <typename Fn, typename Tuple, typename Ix> struct _apply_probe;
template <typename Fn, typename Tuple, ::std::size_t... Ix>
struct _apply_probe<Fn, Tuple, ::std::index_sequence<Ix...>> {
  static constexpr bool _applicable = ::std::is_invocable_v<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))...>;
  static constexpr bool _nothrow
      = ::std::is_nothrow_invocable_v<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))...>;
  using _apply_result = ::std::invoke_result<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))...>;
};

template <bool, typename Fn, typename Tuple> struct _apply_gate { // not tuple-like, or protocol unavailable
  static constexpr bool _applicable = false;
  static constexpr bool _nothrow = false;
  struct _apply_result {}; // no member `type`, [meta.trans.other]
};
template <typename Fn, typename Tuple>
struct _apply_gate<true, Fn, Tuple>
    : _apply_probe<Fn, Tuple, ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Tuple>>>> {};

template <typename Fn, typename Tuple>
using _apply_traits = _apply_gate<_tuple_like<Tuple> && _tuple_sized<Tuple>, Fn, Tuple>;

template <typename Fn, typename Tuple, ::std::size_t... Ix>
constexpr decltype(auto) _apply_impl(Fn &&fn, Tuple &&t, ::std::index_sequence<Ix...>) //
    noexcept(noexcept(::std::invoke(::std::forward<Fn>(fn), ::std::get<Ix>(::std::forward<Tuple>(t))...)))
{
  return ::std::invoke(::std::forward<Fn>(fn), ::std::get<Ix>(::std::forward<Tuple>(t))...);
}

} // namespace detail

// [meta.rel] and [meta.trans.other], as adopted for C++26 by P1317R2
template <typename Fn, typename Tuple>
struct is_applicable : ::std::bool_constant<detail::_apply_traits<Fn, Tuple>::_applicable> {};
template <typename Fn, typename Tuple> constexpr inline bool is_applicable_v = is_applicable<Fn, Tuple>::value;

template <typename Fn, typename Tuple>
struct is_nothrow_applicable : ::std::bool_constant<detail::_apply_traits<Fn, Tuple>::_nothrow> {};
template <typename Fn, typename Tuple>
constexpr inline bool is_nothrow_applicable_v = is_nothrow_applicable<Fn, Tuple>::value;

template <typename Fn, typename Tuple> struct apply_result : detail::_apply_traits<Fn, Tuple>::_apply_result {};
template <typename Fn, typename Tuple> using apply_result_t = typename apply_result<Fn, Tuple>::type;

// [tuple.apply] in its C++26 shape: the declared return type makes the overload SFINAE-friendly
// where C++20's deduced return is a hard error outside the immediate context, and the exception
// specification is the trait's answer.
template <typename Fn, detail::_tuple_like Tuple>
constexpr apply_result_t<Fn, Tuple> apply(Fn &&fn, Tuple &&t) noexcept(is_nothrow_applicable_v<Fn, Tuple>)
{
  return detail::_apply_impl(::std::forward<Fn>(fn), ::std::forward<Tuple>(t),
                             ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Tuple>>>{});
}

} // namespace pfn

#endif // INCLUDE_PFN_TUPLE
