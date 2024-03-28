// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_FUNCTIONAL
#define INCLUDE_FUNCTIONAL_FUNCTIONAL

#include "functional/detail/functional.hpp"

#include <functional>
#include <type_traits>

namespace fn {

// invoke_result and invoke_result_t
template <typename Fn, typename... Args> struct invoke_result : detail::_invoke_result<Fn, Args...> {};
template <typename Fn, typename... Args> using invoke_result_t = typename invoke_result<Fn, Args...>::type;

// transform_result and transform_result_t
template <typename Fn, typename... Args> struct transform_result : detail::_transform_result<Fn, Args...> {};
template <typename Fn, typename... Args> using transform_result_t = typename transform_result<Fn, Args...>::type;

// is_invocable and is_invocable_v
template <typename Fn, typename... Args> struct is_invocable : detail::_is_invocable<Fn, Args...> {};
template <typename Fn, typename... Args> constexpr inline bool is_invocable_v = is_invocable<Fn, Args...>::value;

// is_invocable_r and is_invocable_r_v
template <typename Ret, typename Fn, typename... Args>
struct is_invocable_r : detail::_is_invocable_r<Ret, Fn, Args...> {};
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool is_invocable_r_v = is_invocable_r<Ret, Fn, Args...>::value;

// is_nothrow_invocable and is_nothrow_invocable_v
template <typename Fn, typename... Args> struct is_nothrow_invocable : detail::_is_nothrow_invocable<Fn, Args...> {};
template <typename Fn, typename... Args>
constexpr inline bool is_nothrow_invocable_v = is_nothrow_invocable<Fn, Args...>::value;

// is_nothrow_invocable_r and is_nothrow_invocable_r_v
template <typename Ret, typename Fn, typename... Args>
struct is_nothrow_invocable_r : detail::_is_nothrow_invocable_r<Ret, Fn, Args...> {};
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool is_nothrow_invocable_r_v = is_nothrow_invocable_r<Ret, Fn, Args...>::value;

// invoke
template <typename Fn, typename... Args>
  requires is_invocable_v<Fn, Args...>
constexpr inline auto invoke(Fn &&fn, Args &&...args) noexcept(is_nothrow_invocable_v<Fn, Args...>)
    -> invoke_result_t<Fn, Args...>
{
  return detail::_invoke(FWD(fn), FWD(args)...);
}

// invoke_r
template <typename Ret, typename Fn, typename... Args>
  requires is_invocable_r_v<Ret, Fn, Args...>
constexpr auto invoke_r(Fn &&fn, Args &&...args) noexcept(is_nothrow_invocable_r_v<Ret, Fn, Args...>) -> Ret
{
  return detail::_invoke_r<Ret>(FWD(fn), FWD(args)...);
}

// invocable and regular_invocable
template <typename Fn, typename... Args>
concept invocable = is_invocable_v<Fn, Args...>;

template <typename Fn, typename... Args>
concept regular_invocable = invocable<Fn, Args...>;

template <typename Fn, typename T>
concept typelist_invocable = detail::_typelist_invocable<Fn, T>;
template <typename Ret, typename Fn, typename T>
concept typelist_invocable_r = detail::_typelist_invocable_r<Ret, Fn, T>;

template <typename Fn, typename T>
concept typelist_type_invocable = detail::_typelist_type_invocable<Fn, T>;
template <typename Ret, typename Fn, typename T>
concept typelist_type_invocable_r = detail::_typelist_type_invocable_r<Ret, Fn, T>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_FUNCTIONAL
