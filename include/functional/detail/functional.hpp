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
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _invoke_result<Fn, Args...> : ::std::invoke_result<Fn, Args...> {};

template <typename Fn, typename Arg>
constexpr auto _invoke_result_result(Fn &&, Arg &&)
    -> std::type_identity<decltype(std::declval<Arg>().invoke(std::declval<Fn>()))>;
template <typename Fn, typename Arg> constexpr auto _invoke_result_result(auto &&...) -> std::type_identity<void>;

template <typename Fn, typename Arg>
  requires _some_pack<Arg>
struct _invoke_result<Fn, Arg> {
  using type = decltype(_invoke_result_result<Fn, Arg>(std::declval<Fn>(), std::declval<Arg>()))::type;
};
template <typename Fn, typename Arg>
  requires _some_sum<Arg>
struct _invoke_result<Fn, Arg> {
  using type = decltype(_invoke_result_result<Fn, Arg>(std::declval<Fn>(), std::declval<Arg>()))::type;
};
template <typename Fn, typename... Args> using _invoke_result_t = typename _invoke_result<Fn, Args...>::type;

// is_invocable
template <typename Fn, typename... Args> struct _is_invocable;
template <typename Fn, typename... Args>
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_invocable<Fn, Args...> : ::std::is_invocable<Fn, Args...> {};

template <typename Fn, typename Arg>
constexpr auto _is_invocable_result(Fn &&, Arg &&,
                                    std::type_identity<decltype(std::declval<Arg>().invoke(std::declval<Fn>()))> = {})
    -> std::true_type;
template <typename Fn, typename Arg> constexpr auto _is_invocable_result(auto &&...) -> std::false_type;
template <typename Fn, typename Arg>
  requires _some_pack<Arg>
struct _is_invocable<Fn, Arg> {
  static constexpr bool value = decltype(_is_invocable_result<Fn, Arg>(std::declval<Fn>(), std::declval<Arg>()))::value;
};
template <typename Fn, typename Arg>
  requires _some_sum<Arg>
struct _is_invocable<Fn, Arg> {
  static constexpr bool value = decltype(_is_invocable_result<Fn, Arg>(std::declval<Fn>(), std::declval<Arg>()))::value;
};
template <typename Fn, typename... Args> constexpr inline bool _is_invocable_v = _is_invocable<Fn, Args...>::value;

// is_invocable_r
template <typename Ret, typename Fn, typename... Args> struct _is_invocable_r;
template <typename Ret, typename Fn, typename... Args>
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_invocable_r<Ret, Fn, Args...> : ::std::is_invocable_r<Ret, Fn, Args...> {};

template <bool, typename Ret, typename Fn, typename Arg> struct _is_invocable_r_result;
template <typename Ret, typename Fn, typename Arg> struct _is_invocable_r_result<false, Ret, Fn, Arg> {
  static constexpr bool value = false;
};
template <typename Ret, typename Fn, typename Arg> struct _is_invocable_r_result<true, Ret, Fn, Arg> {
  static constexpr bool value = ::std::is_convertible_v<_invoke_result_t<Fn, Arg>, Ret>;
};
template <typename Ret, typename Fn, typename Arg>
  requires _some_pack<Arg>
struct _is_invocable_r<Ret, Fn, Arg> {
  static constexpr bool value = _is_invocable_r_result<_is_invocable<Fn, Arg>::value, Ret, Fn, Arg>::value;
};
template <typename Ret, typename Fn, typename Arg>
  requires _some_sum<Arg>
struct _is_invocable_r<Ret, Fn, Arg> {
  static constexpr bool value = (requires(Fn fn, Arg arg) {
    invoke_variadic_union<Ret, typename std::remove_cvref_t<Arg>::data_t>(arg.data, 0, fn);
  });
};
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool _is_invocable_r_v = _is_invocable_r<Ret, Fn, Args...>::value;

// is_nothrow_invocable and is_nothrow_invocable_v
template <typename Fn, typename... Args> struct _is_nothrow_invocable;
template <typename Fn, typename... Args>
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_nothrow_invocable<Fn, Args...> : ::std::is_nothrow_invocable<Fn, Args...> {};
template <typename Fn, typename Arg>
  requires _some_pack<Arg>
struct _is_nothrow_invocable<Fn, Arg> {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = _is_invocable<Fn, Arg>::value;
};
template <typename Fn, typename Arg>
  requires _some_sum<Arg>
struct _is_nothrow_invocable<Fn, Arg> {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = _is_invocable<Fn, Arg>::value;
};
template <typename Fn, typename... Args>
constexpr inline bool _is_nothrow_invocable_v = _is_nothrow_invocable<Fn, Args...>::value;

// is_nothrow_invocable_r and is_nothrow_invocable_r_v
template <typename Ret, typename Fn, typename... Args> struct _is_nothrow_invocable_r;
template <typename Ret, typename Fn, typename... Args>
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
struct _is_nothrow_invocable_r<Ret, Fn, Args...> : ::std::is_nothrow_invocable_r<Ret, Fn, Args...> {};
template <typename Ret, typename Fn, typename Arg>
  requires _some_pack<Arg>
struct _is_nothrow_invocable_r<Ret, Fn, Arg> {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = _is_invocable_r<Ret, Fn, Arg>::value;
};
template <typename Ret, typename Fn, typename Arg>
  requires _some_sum<Arg>
struct _is_nothrow_invocable_r<Ret, Fn, Arg> {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = _is_invocable_r<Ret, Fn, Arg>::value;
};
template <typename Ret, typename Fn, typename... Args>
constexpr inline bool _is_nothrow_invocable_r_v = _is_nothrow_invocable_r<Ret, Fn, Args...>::value;

// invoke
template <typename Fn, typename... Args>
  requires(sizeof...(Args) != 1)
          || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>))
constexpr inline _invoke_result_t<Fn, Args...> _invoke(Fn &&fn,
                                                       Args &&...args) noexcept(_is_nothrow_invocable_v<Fn, Args...>)
{
  return ::std::invoke(FWD(fn), FWD(args)...);
}
template <typename Fn, typename Arg>
  requires _some_pack<Arg>
constexpr inline _invoke_result_t<Fn, Arg> _invoke(Fn &&fn, Arg &&arg) noexcept(_is_nothrow_invocable_v<Fn, Arg>)
{
  return FWD(arg).invoke(FWD(fn));
}
template <typename Fn, typename Arg>
  requires _some_sum<Arg>
constexpr inline _invoke_result_t<Fn, Arg> _invoke(Fn &&fn, Arg &&arg) noexcept(_is_nothrow_invocable_v<Fn, Arg>)
{
  return FWD(arg).invoke(FWD(fn));
}

// invoke_r
template <typename Ret, typename Fn, typename... Args>
  requires((sizeof...(Args) != 1)
           || ((not _some_pack<detail::select_nth_t<0, Args...>>) && (not _some_sum<detail::select_nth_t<0, Args...>>)))
          && _is_invocable_r_v<Ret, Fn, Args...>
constexpr Ret _invoke_r(Fn &&fn, Args &&...args) noexcept(_is_nothrow_invocable_r_v<Ret, Fn, Args...>)
{
  return ::std::invoke_r<Ret>(FWD(fn), FWD(args)...);
}
template <typename Ret, typename Fn, typename Arg>
  requires _some_pack<Arg> && _is_invocable_r_v<Ret, Fn, Arg>
constexpr Ret _invoke_r(Fn &&fn, Arg &&arg) noexcept(_is_nothrow_invocable_r_v<Ret, Fn, Arg>)
{
  return FWD(arg).invoke(FWD(fn));
}
template <typename Ret, typename Fn, typename Arg>
  requires _some_sum<Arg> && _is_invocable_r_v<Ret, Fn, Arg>
constexpr Ret _invoke_r(Fn &&fn, Arg &&arg) noexcept(_is_nothrow_invocable_r_v<Ret, Fn, Arg>)
{
  return FWD(arg).template invoke_r<Ret>(FWD(fn));
}

template <typename Fn, typename T> constexpr inline bool _is_ts_invocable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> &> = (... && _is_invocable_v<Fn, Ts &>);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> const &> = (... && _is_invocable_v<Fn, Ts const &>);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> &&> = (... && _is_invocable_v<Fn, Ts &&>);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_ts_invocable<Fn, Tpl<Ts...> const &&> = (... && _is_invocable_v<Fn, Ts const &&>);
template <typename Fn, typename T>
concept _typelist_invocable = _is_ts_invocable<Fn, T &&>;

template <typename R, typename Fn, typename T> constexpr inline bool _is_rts_invocable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> &> = (... && _is_invocable_r_v<R, Fn, Ts &>);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> const &> = (... && _is_invocable_r_v<R, Fn, Ts const &>);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> &&> = (... && _is_invocable_r_v<R, Fn, Ts &&>);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rts_invocable<R, Fn, Tpl<Ts...> const &&> = (... && _is_invocable_r_v<R, Fn, Ts const &&>);
template <typename R, typename Fn, typename T>
concept _typelist_invocable_r = _is_rts_invocable<R, Fn, T &&>;

template <typename Fn, typename T> constexpr inline bool _is_tst_invocable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> &>
    = (... && _is_invocable_v<Fn, ::std::in_place_type_t<Ts>, Ts &>);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> const &>
    = (... && _is_invocable_v<Fn, ::std::in_place_type_t<Ts>, Ts const &>);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> &&>
    = (... && _is_invocable_v<Fn, ::std::in_place_type_t<Ts>, Ts &&>);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> const &&>
    = (... && _is_invocable_v<Fn, ::std::in_place_type_t<Ts>, Ts const &&>);
template <typename Fn, typename T>
concept _typelist_type_invocable = _is_tst_invocable<Fn, T &&>;

template <typename R, typename Fn, typename T> constexpr inline bool _is_rtst_invocable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> &>
    = (... && _is_invocable_r_v<R, Fn, ::std::in_place_type_t<Ts>, Ts &>);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> const &>
    = (... && _is_invocable_r_v<R, Fn, ::std::in_place_type_t<Ts>, Ts const &>);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> &&>
    = (... && _is_invocable_r_v<R, Fn, ::std::in_place_type_t<Ts>, Ts &&>);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> const &&>
    = (... && _is_invocable_r_v<R, Fn, ::std::in_place_type_t<Ts>, Ts const &&>);
template <typename R, typename Fn, typename T>
concept _typelist_type_invocable_r = _is_rtst_invocable<R, Fn, T &&>;
} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_FUNCTIONAL
