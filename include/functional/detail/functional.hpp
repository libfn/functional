// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_FUNCTIONAL
#define INCLUDE_FUNCTIONAL_DETAIL_FUNCTIONAL

#include "functional/detail/fwd.hpp"
#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/meta.hpp"

#include <functional>
#include <type_traits>
#include <utility>

namespace fn::detail {

// invoke_result
template <typename Fn, typename... Args> struct _invoke_result;
template <typename Fn, typename... Args>
  requires(sizeof...(Args) == 0)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _invoke_result<Fn, Args...> : ::std::invoke_result<Fn, Args...> {};

template <typename Fn, typename Arg, typename... Args>
constexpr auto _invoke_result_result(Fn &&, Arg &&, Args...)
    -> std::type_identity<decltype(std::declval<Arg>().invoke(std::declval<Fn>(), std::declval<Args>()...))>;
template <typename Fn, typename Arg, typename... Args>
constexpr auto _invoke_result_result(auto &&...) -> std::type_identity<void>;
template <typename Fn, typename Arg, typename... Args>
  requires _some_pack<Arg> || _some_sum<Arg>
struct _invoke_result<Fn, Arg, Args...> {
  using type = decltype(_invoke_result_result<Fn, Arg, Args...>(std::declval<Fn>(), std::declval<Arg>(),
                                                                std::declval<Args>()...))::type;
};

// is_invocable
template <typename Fn, typename... Args> struct _is_invocable;
template <typename Fn, typename... Args>
  requires(sizeof...(Args) == 0)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_invocable<Fn, Args...> : ::std::is_invocable<Fn, Args...> {};

template <typename Fn, typename Arg, typename... Args>
constexpr auto _is_invocable_result(
    Fn &&, Arg &&, Args &&...,
    std::type_identity<decltype(std::declval<Arg>().invoke(std::declval<Fn>(), std::declval<Args>()...))> = {})
    -> std::true_type;
template <typename Fn, typename Arg, typename... Args>
constexpr auto _is_invocable_result(auto &&...) -> std::false_type;
template <typename Fn, typename Arg, typename... Args>
  requires _some_pack<Arg> || _some_sum<Arg>
struct _is_invocable<Fn, Arg, Args...> {
  static constexpr bool value = decltype(_is_invocable_result<Fn, Arg, Args...>(std::declval<Fn>(), std::declval<Arg>(),
                                                                                std::declval<Args>()...))::value;
};

// is_invocable_r
template <typename Ret, typename Fn, typename... Args> struct _is_invocable_r;
template <typename Ret, typename Fn, typename... Args>
  requires(sizeof...(Args) == 0)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_invocable_r<Ret, Fn, Args...> : ::std::is_invocable_r<Ret, Fn, Args...> {};

template <typename Ret, typename Fn, typename Arg, typename... Args>
constexpr auto
_is_invocable_r_result(Fn &&, Arg &&, Args &&...,
                       std::type_identity<decltype(std::declval<Arg>().template invoke_r<Ret>(std::declval<Fn>(),
                                                                                              std::declval<Args>()...))>
                       = {}) -> std::true_type;
template <typename Ret, typename Fn, typename Arg, typename... Args>
constexpr auto _is_invocable_r_result(auto &&...) -> std::false_type;
template <typename Ret, typename Fn, typename Arg, typename... Args>
  requires _some_pack<Arg> || _some_sum<Arg>
struct _is_invocable_r<Ret, Fn, Arg, Args...> {
  static constexpr bool value = decltype(_is_invocable_r_result<Ret, Fn, Arg, Args...>(
      std::declval<Fn>(), std::declval<Arg>(), std::declval<Args>()...))::value;
};

// is_nothrow_invocable and is_nothrow_invocable_v
template <typename Fn, typename... Args> struct _is_nothrow_invocable;
template <typename Fn, typename... Args>
  requires(sizeof...(Args) == 0)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_nothrow_invocable<Fn, Args...> : ::std::is_nothrow_invocable<Fn, Args...> {};
template <typename Fn, typename Arg, typename... Args>
  requires _some_pack<Arg> || _some_sum<Arg>
struct _is_nothrow_invocable<Fn, Arg, Args...> {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = _is_invocable<Fn, Arg, Args...>::value;
};

// is_nothrow_invocable_r and is_nothrow_invocable_r_v
template <typename Ret, typename Fn, typename... Args> struct _is_nothrow_invocable_r;
template <typename Ret, typename Fn, typename... Args>
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_nothrow_invocable_r<Ret, Fn, Args...> : ::std::is_nothrow_invocable_r<Ret, Fn, Args...> {};
template <typename Ret, typename Fn, typename Arg, typename... Args>
  requires _some_pack<Arg> || _some_sum<Arg>
struct _is_nothrow_invocable_r<Ret, Fn, Arg, Args...> {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = _is_invocable_r<Ret, Fn, Arg, Args...>::value;
};

// invoke
template <typename Fn, typename... Args>
  requires((sizeof...(Args) == 0)
           || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>)))
          && _is_invocable<Fn, Args...>::value
constexpr inline _invoke_result<Fn, Args...>::type
_invoke(Fn &&fn, Args &&...args) noexcept(_is_nothrow_invocable<Fn, Args...>::value)
{
  return ::std::invoke(FWD(fn), FWD(args)...);
}
template <typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>) && _is_invocable<Fn, Arg, Args...>::value
constexpr inline _invoke_result<Fn, Arg, Args...>::type
_invoke(Fn &&fn, Arg &&arg, Args &&...args) noexcept(_is_nothrow_invocable<Fn, Arg, Args...>::value)
{
  return FWD(arg).invoke(FWD(fn), FWD(args)...);
}

// invoke_r
template <typename Ret, typename Fn, typename... Args>
  requires((sizeof...(Args) != 1)
           || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>)))
          && _is_invocable_r<Ret, Fn, Args...>::value
constexpr Ret _invoke_r(Fn &&fn, Args &&...args) noexcept(_is_nothrow_invocable_r<Ret, Fn, Args...>::value)
{
  return ::std::invoke_r<Ret>(FWD(fn), FWD(args)...);
}
template <typename Ret, typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>) && _is_invocable_r<Ret, Fn, Arg, Args...>::value
constexpr Ret _invoke_r(Fn &&fn, Arg &&arg,
                        Args &&...args) noexcept(_is_nothrow_invocable_r<Ret, Fn, Arg, Args...>::value)
{
  return FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Fn, typename T, typename... Tx> constexpr inline bool _is_ts_invocable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> &, Tx...> = (... && _is_invocable<Fn, Ts &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_invocable<Fn, Ts const &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> &&, Tx...> = (... && _is_invocable<Fn, Ts &&, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_invocable<Fn, Ts const &&, Tx...>::value);
template <typename Fn, typename T, typename... Tx>
concept _typelist_invocable = _is_ts_invocable<Fn, T &&, Tx...>;

template <typename R, typename Fn, typename T, typename... Tx> constexpr inline bool _is_rts_invocable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> &, Tx...>
    = (... && _is_invocable_r<R, Fn, Ts &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_invocable_r<R, Fn, Ts const &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> &&, Tx...>
    = (... && _is_invocable_r<R, Fn, Ts &&, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_invocable_r<R, Fn, Ts const &&, Tx...>::value);
template <typename R, typename Fn, typename T, typename... Tx>
concept _typelist_invocable_r = _is_rts_invocable<R, Fn, T &&, Tx...>;

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_FUNCTIONAL
