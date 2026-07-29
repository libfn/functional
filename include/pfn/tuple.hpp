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
#include <libfn_version.hpp>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pfn {
inline namespace LIBFN_VERSION_BASE {
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

/**
 * @brief Checks if `Fn` is invocable with the elements of the tuple-like `Tuple`
 *
 * `std::is_applicable` as adopted for C++26 by P1317R2 ([meta.rel]), for C++20 compilers.
 * `Tuple` must be one of the enumerated tuple-like specializations ([tuple.like]); where the
 * underlying standard library does not provide `std::complex`'s C++26 tuple protocol (P2819),
 * a `std::complex` operand answers `false` rather than failing to compile.
 *
 * @tparam Fn Callable to probe
 * @tparam Tuple Tuple-like operand type, possibly cv-ref qualified
 */
template <typename Fn, typename Tuple>
struct is_applicable : ::std::bool_constant<detail::_apply_traits<Fn, Tuple>::_applicable> {};
/**
 * @brief Variable form of `pfn::is_applicable`
 */
template <typename Fn, typename Tuple> constexpr inline bool is_applicable_v = is_applicable<Fn, Tuple>::value;

/**
 * @brief Checks if the `pfn::is_applicable` invocation is additionally known not to throw
 *
 * `std::is_nothrow_applicable` as adopted for C++26 by P1317R2 ([meta.rel]), for C++20 compilers.
 *
 * @tparam Fn Callable to probe
 * @tparam Tuple Tuple-like operand type, possibly cv-ref qualified
 */
template <typename Fn, typename Tuple>
struct is_nothrow_applicable : ::std::bool_constant<detail::_apply_traits<Fn, Tuple>::_nothrow> {};
/**
 * @brief Variable form of `pfn::is_nothrow_applicable`
 */
template <typename Fn, typename Tuple>
constexpr inline bool is_nothrow_applicable_v = is_nothrow_applicable<Fn, Tuple>::value;

/**
 * @brief The result type of applying `Fn` to the elements of the tuple-like `Tuple`
 *
 * `std::apply_result` as adopted for C++26 by P1317R2 ([meta.trans.other]), for C++20 compilers.
 * SFINAE-friendly: no member `type` where `pfn::is_applicable` answers `false`.
 *
 * @tparam Fn Callable to probe
 * @tparam Tuple Tuple-like operand type, possibly cv-ref qualified
 */
template <typename Fn, typename Tuple> struct apply_result : detail::_apply_traits<Fn, Tuple>::_apply_result {};
/**
 * @brief Alias form of `pfn::apply_result`
 */
template <typename Fn, typename Tuple> using apply_result_t = typename apply_result<Fn, Tuple>::type;

/**
 * @brief Invokes a callable with a tuple-like operand's elements as its arguments:
 *        `std::apply` in its C++26 shape ([tuple.apply]), for C++20 compilers
 *
 * The C++26 revision (P1317R2) declares the return type as `apply_result_t` - making the
 * overload SFINAE-friendly where a deduced return is a hard error outside the immediate
 * context - and derives the exception specification from `is_nothrow_applicable`; both are
 * provided here. Accepts the enumerated tuple-like types ([tuple.like]), not the general
 * tuple protocol.
 *
 * @param fn Callable to invoke
 * @param t Tuple-like operand supplying the arguments
 * @return The callable's result
 */
template <typename Fn, detail::_tuple_like Tuple>
constexpr apply_result_t<Fn, Tuple> apply(Fn &&fn, Tuple &&t) noexcept(is_nothrow_applicable_v<Fn, Tuple>)
{
  return detail::_apply_impl(::std::forward<Fn>(fn), ::std::forward<Tuple>(t),
                             ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Tuple>>>{});
}

} // namespace LIBFN_VERSION_BASE
} // namespace pfn

#endif // INCLUDE_PFN_TUPLE
