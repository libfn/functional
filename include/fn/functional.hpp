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

// apply_result and apply_result_t
template <typename Fn, typename... Args> struct apply_result : detail::_apply_result<Fn, Args...> {};
template <typename Fn, typename... Args> using apply_result_t = typename apply_result<Fn, Args...>::type;

// is_applicable and is_applicable_v
template <typename Fn, typename... Args> struct is_applicable : detail::_is_applicable<Fn, Args...> {};
template <typename Fn, typename... Args> constexpr inline bool is_applicable_v = is_applicable<Fn, Args...>::value;

// is_applicable_r and is_applicable_r_v
template <typename Ret, typename Fn, typename... Args>
struct is_applicable_r : detail::_is_applicable_r<Ret, Fn, Args...> {};
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool is_applicable_r_v = is_applicable_r<Ret, Fn, Args...>::value;

// is_nothrow_applicable and is_nothrow_applicable_v
template <typename Fn, typename... Args> struct is_nothrow_applicable : detail::_is_nothrow_applicable<Fn, Args...> {};
template <typename Fn, typename... Args>
constexpr inline bool is_nothrow_applicable_v = is_nothrow_applicable<Fn, Args...>::value;

// is_nothrow_applicable_r and is_nothrow_applicable_r_v
template <typename Ret, typename Fn, typename... Args>
struct is_nothrow_applicable_r : detail::_is_nothrow_applicable_r<Ret, Fn, Args...> {};
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool is_nothrow_applicable_r_v = is_nothrow_applicable_r<Ret, Fn, Args...>::value;

// apply
template <typename Fn, typename... Args>
  requires is_applicable_v<Fn, Args...>
constexpr auto apply(Fn &&fn, Args &&...args) noexcept(is_nothrow_applicable_v<Fn, Args...>)
    -> apply_result_t<Fn, Args...>
{
  return detail::_apply(FWD(fn), FWD(args)...);
}

// apply_r
template <typename Ret, typename Fn, typename... Args>
  requires is_applicable_r_v<Ret, Fn, Args...>
constexpr auto apply_r(Fn &&fn, Args &&...args) noexcept(is_nothrow_applicable_r_v<Ret, Fn, Args...>) -> Ret
{
  return detail::_apply_r<Ret>(FWD(fn), FWD(args)...);
}

// applicable and regular_applicable
template <typename Fn, typename... Args>
concept applicable = is_applicable_v<Fn, Args...>;

template <typename Fn, typename... Args>
concept regular_applicable = applicable<Fn, Args...>;

template <typename Fn, typename T, typename... Args>
concept typelist_applicable = detail::_typelist_applicable<Fn, T, Args...>;
template <typename Ret, typename Fn, typename T, typename... Args>
concept typelist_applicable_r = detail::_typelist_applicable_r<Ret, Fn, T, Args...>;

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_FUNCTIONAL
