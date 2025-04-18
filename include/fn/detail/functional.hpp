// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_FUNCTIONAL
#define INCLUDE_FN_DETAIL_FUNCTIONAL

#include <fn/detail/fwd.hpp>
#include <fn/detail/fwd_macro.hpp>
#include <fn/detail/meta.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace fn::detail {

namespace _fold_detail {
template <typename L, typename R> [[nodiscard]] constexpr auto _fold(auto &&l, auto &&r)
{
  if constexpr (_some_pack<L>) {
    return FWD(l).append(std::in_place_type_t<R>{}, FWD(r));
  } else {
    if constexpr (_some_pack<R>) {
      return ::fn::pack<L>{FWD(l)}.append(std::in_place_type_t<R>{}, FWD(r));
    } else {
      return ::fn::pack<L, R>{FWD(l), FWD(r)};
    }
  }
}

template <typename Lh, typename Rh>
  requires _some_sum<Lh> && _some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(lv)._transform([&rv]<typename L>(std::in_place_type_t<L>, auto &&l) {
    return FWD(rv)._transform(
        [&l]<typename R>(std::in_place_type_t<R>, auto &&r) { return _fold<L, R>(FWD(l), FWD(r)); });
  });
}

template <typename Lh, typename Rh>
  requires _some_sum<Lh> && (not _some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(lv)._transform(
      [&rv]<typename L>(std::in_place_type_t<L>, auto &&l) { return _fold<L, Rh>(FWD(l), FWD(rv)); });
}

template <typename Lh, typename Rh>
  requires(not _some_sum<Lh>) && _some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(rv)._transform(
      [&lv]<typename R>(std::in_place_type_t<R>, auto &&r) { return _fold<Lh, R>(FWD(lv), FWD(r)); });
}

template <typename Lh, typename Rh>
  requires(not _some_sum<Lh>) && (not _some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return _fold<Lh, Rh>(FWD(lv), FWD(rv));
}
} // namespace _fold_detail

namespace _invoke_detail {
template <typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_sum<Args>))) && ::std::is_invocable_v<Fn, Args...>
[[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) -> decltype(auto)
{
  return ::std::invoke(FWD(fn), FWD(args)...);
}

template <typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>)
              && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_sum<Args>))))
              && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).invoke(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto invoke(Fn &&fn, Arg &&arg, Args &&...args) -> decltype(auto)

{
  return FWD(arg).invoke(FWD(fn), FWD(args)...);
}

template <typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_sum<Arg0>) || ... || (_some_pack<Args> || _some_sum<Args>))
              && requires(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) {
                   invoke<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
                       FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
                 }
[[nodiscard]] constexpr auto invoke(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) -> decltype(auto)
{
  using type = decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)));
  return invoke<Fn, type, Args...>(FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)),
                                   FWD(args)...);
}

template <typename Ret, typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_sum<Args>))) && ::std::is_invocable_r_v<Ret, Fn, Args...>
[[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) -> decltype(auto)
{
  return ::std::invoke_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Ret, typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>)
              && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_sum<Args>))))
              && requires(Fn &&fn, Arg &&arg, Args &&...args) {
                   FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...);
                 }
[[nodiscard]] constexpr auto invoke_r(Fn &&fn, Arg &&arg, Args &&...args) -> decltype(auto)
{
  return FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Ret, typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_sum<Arg0>) || ... || (_some_pack<Args> || _some_sum<Args>))
              && requires(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) {
                   invoke_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))),
                            Args...>(FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)),
                                     FWD(args)...);
                 }
[[nodiscard]] constexpr auto invoke_r(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) -> decltype(auto)
{
  using type = decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)));
  return invoke_r<Ret, Fn, type, Args...>(FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)),
                                          FWD(args)...);
}
} // namespace _invoke_detail

// invoke_result
template <typename Fn, typename... Args>
constexpr auto _invoke_result_result(Fn &&, Args &&...)
    -> std::type_identity<decltype(_invoke_detail::invoke(std::declval<Fn>(), std::declval<Args>()...))>;
template <typename Fn, typename... Args> constexpr auto _invoke_result_result(auto &&...) -> std::type_identity<void>;

template <typename Fn, typename... Args> struct _invoke_result {
  using type = decltype(_invoke_result_result<Fn, Args...>(std::declval<Fn>(), std::declval<Args>()...))::type;
};

// is_invocable
template <typename Fn, typename... Args>
constexpr auto _is_invocable_result(Fn &&, Args &&...,
                                    std::type_identity<decltype(::fn::detail::_invoke_detail::invoke<Fn, Args...>(
                                        std::declval<Fn>(), std::declval<Args>()...))>
                                    = {}) -> std::true_type;
template <typename Fn, typename... Args> constexpr auto _is_invocable_result(auto &&...) -> std::false_type;

template <typename Fn, typename... Args> struct _is_invocable {
  static constexpr bool value
      = decltype(_is_invocable_result<Fn, Args...>(std::declval<Fn>(), std::declval<Args>()...))::value;
};

// is_invocable_r
template <typename Ret, typename Fn, typename... Args>
constexpr auto _is_invocable_r_result(
    Fn &&, Args &&...,
    std::type_identity<decltype(_invoke_detail::invoke_r<Ret>(std::declval<Fn>(), std::declval<Args>()...))>
    = {}) -> std::true_type;
template <typename Ret, typename Fn, typename... Args>
constexpr auto _is_invocable_r_result(auto &&...) -> std::false_type;
template <typename Ret, typename Fn, typename... Args> struct _is_invocable_r {
  static constexpr bool value
      = decltype(_is_invocable_r_result<Ret, Fn, Args...>(std::declval<Fn>(), std::declval<Args>()...))::value;
};

// is_nothrow_invocable and is_nothrow_invocable_v
template <typename Fn, typename... Args> struct _is_nothrow_invocable {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = false;
};

// is_nothrow_invocable_r and is_nothrow_invocable_r_v
template <typename Ret, typename Fn, typename... Args> struct _is_nothrow_invocable_r {
  // TODO https://github.com/libfn/functional/issues/45
  static constexpr bool value = false;
};

// invoke
template <typename Fn, typename... Args>
  requires(_is_invocable<Fn, Args...>::value)
constexpr inline auto _invoke(Fn &&fn, Args &&...args) noexcept(_is_nothrow_invocable<Fn, Args...>::value)
    -> _invoke_result<Fn, Args...>::type
{
  return _invoke_detail::invoke(FWD(fn), FWD(args)...);
}

// invoke_r
template <typename Ret, typename Fn, typename... Args>
  requires(_is_invocable_r<Ret, Fn, Args...>::value)
constexpr auto _invoke_r(Fn &&fn, Args &&...args) noexcept(_is_nothrow_invocable_r<Ret, Fn, Args...>::value) -> Ret
{
  return _invoke_detail::invoke_r<Ret>(FWD(fn), FWD(args)...);
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
