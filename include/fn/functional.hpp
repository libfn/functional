// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_FUNCTIONAL
#define INCLUDE_FN_FUNCTIONAL

#include <fn/detail/functional.hpp>
#include <libfn_version.hpp>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

/**
 * @brief The result type of `fn::apply` over the arguments
 *
 * Yields `void` where the application is not viable, as well as for a viable application returning
 * `void` - pair with `is_applicable` to tell the two apart.
 *
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply` would take them, including `pack` and `copack` operands
 */
template <typename Fn, typename... Args> struct apply_result : detail::_apply_result<Fn, Args...> {};

/**
 * @brief Alias for `apply_result<Fn, Args...>::type`
 */
template <typename Fn, typename... Args> using apply_result_t = typename apply_result<Fn, Args...>::type;

/**
 * @brief Checks if `fn::apply` of `Fn` over the arguments is viable
 *
 * The multidispatch twin of `std::is_invocable`: a `pack` operand counts by its elements, and a
 * `copack` operand counts when every alternative can be dispatched. A set of alternatives which
 * disagree on the result type is rejected by a `static_assert` inside the dispatch, not by this
 * trait answering false.
 *
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply` would take them
 */
template <typename Fn, typename... Args> struct is_applicable : detail::_is_applicable<Fn, Args...> {};

/**
 * @brief Alias for `is_applicable<Fn, Args...>::value`
 */
template <typename Fn, typename... Args> constexpr inline bool is_applicable_v = is_applicable<Fn, Args...>::value;

/**
 * @brief Checks if `fn::apply_r<Ret>` of `Fn` over the arguments is viable
 *
 * As `is_applicable`, except every branch result needs only to be acceptable as `Ret` rather than
 * converge on one deduced type.
 *
 * @tparam Ret Type the results convert to
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply_r` would take them
 */
template <typename Ret, typename Fn, typename... Args>
struct is_applicable_r : detail::_is_applicable_r<Ret, Fn, Args...> {};

/**
 * @brief Alias for `is_applicable_r<Ret, Fn, Args...>::value`
 */
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool is_applicable_r_v = is_applicable_r<Ret, Fn, Args...>::value;

/**
 * @brief Checks if `fn::apply` of `Fn` over the arguments is viable and cannot throw
 *
 * Composes through the dispatch: for a `copack` operand every alternative's call must be nothrow,
 * since which one runs is a run-time fact. Answers false where the call is not viable at all.
 *
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply` would take them
 */
template <typename Fn, typename... Args> struct is_nothrow_applicable : detail::_is_nothrow_applicable<Fn, Args...> {};

/**
 * @brief Alias for `is_nothrow_applicable<Fn, Args...>::value`
 */
template <typename Fn, typename... Args>
constexpr inline bool is_nothrow_applicable_v = is_nothrow_applicable<Fn, Args...>::value;

/**
 * @brief Checks if `fn::apply_r<Ret>` of `Fn` over the arguments is viable and cannot throw
 *
 * @tparam Ret Type the results convert to
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply_r` would take them
 */
template <typename Ret, typename Fn, typename... Args>
struct is_nothrow_applicable_r : detail::_is_nothrow_applicable_r<Ret, Fn, Args...> {};

/**
 * @brief Alias for `is_nothrow_applicable_r<Ret, Fn, Args...>::value`
 */
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool is_nothrow_applicable_r_v = is_nothrow_applicable_r<Ret, Fn, Args...>::value;

/**
 * @brief The multidispatch entry point: unpacks products, dispatches over alternatives, invokes `fn`
 *
 * A `pack` operand spreads into the call by elements; a lone tuple-like argument has `std::apply`'s
 * meaning; a `copack` operand dispatches on its active alternative, itself unpacked one level when
 * tuple-like; several pack/copack operands first fold into one, distributing alternatives over
 * products. The arm is selected by ordinary C++ overload resolution, and the dispatch is
 * exhaustive: an alternative without a viable arm makes the whole call not applicable.
 *
 * @param fn Callable; `fn::overload` fuses per-alternative arms into one
 * @param args Any mix of scalars, `pack`s and `copack`s, in call order
 * @return Result of invoking `fn` on the selected call shape
 */
template <typename Fn, typename... Args>
  requires is_applicable_v<Fn, Args...>
constexpr auto apply(Fn &&fn, Args &&...args) noexcept(is_nothrow_applicable_v<Fn, Args...>)
    -> apply_result_t<Fn, Args...>
{
  return detail::_apply(FWD(fn), FWD(args)...);
}

/**
 * @brief `fn::apply` with the result converted to `Ret`
 *
 * Branch results need only be acceptable as `Ret`, which is how alternatives eliminating into
 * different types converge: name a type they all convert into - every alternative converts
 * implicitly into its parent `copack`, so a `copack_for` of the branch results always serves.
 *
 * @tparam Ret Type the result converts to
 * @param fn Callable; `fn::overload` fuses per-alternative arms into one
 * @param args Any mix of scalars, `pack`s and `copack`s, in call order
 * @return Result of the dispatch, converted to `Ret`
 */
template <typename Ret, typename Fn, typename... Args>
  requires is_applicable_r_v<Ret, Fn, Args...>
constexpr auto apply_r(Fn &&fn, Args &&...args) noexcept(is_nothrow_applicable_r_v<Ret, Fn, Args...>) -> Ret
{
  return detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
}

/**
 * @brief Checks if `fn::apply` of `Fn` over `Args` is viable - the concept form of `is_applicable`
 *
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply` would take them
 */
template <typename Fn, typename... Args>
concept applicable = is_applicable_v<Fn, Args...>;

/**
 * @brief As `applicable`, adding the semantic promise of equality preservation - mirroring
 *        `std::regular_invocable` against `std::invocable`
 *
 * @tparam Fn Callable to apply
 * @tparam Args Arguments as `fn::apply` would take them
 */
template <typename Fn, typename... Args>
concept regular_applicable = applicable<Fn, Args...>;

/**
 * @brief Checks if `Fn` can be applied to every alternative of the typelist `T`
 *
 * The exhaustiveness question `copack` and `choice` dispatch asks before selecting an arm: each
 * alternative is tried in `T`'s own cv-ref qualification, with the trailing `Args` following the
 * alternative's content.
 *
 * @tparam Fn Callable to apply
 * @tparam T The typelist - a `copack` or `choice`, cv-ref qualified
 * @tparam Args Trailing arguments, appended after the alternative's content
 */
template <typename Fn, typename T, typename... Args>
concept typelist_applicable = detail::_typelist_applicable<Fn, T, Args...>;

/**
 * @brief As `typelist_applicable`, with every branch result converting to `Ret`
 *
 * @tparam Ret Type the results convert to
 * @tparam Fn Callable to apply
 * @tparam T The typelist - a `copack` or `choice`, cv-ref qualified
 * @tparam Args Trailing arguments, appended after the alternative's content
 */
template <typename Ret, typename Fn, typename T, typename... Args>
concept typelist_applicable_r = detail::_typelist_applicable_r<Ret, Fn, T, Args...>;

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_FUNCTIONAL
