// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_FUNCTIONAL
#define INCLUDE_FN_DETAIL_FUNCTIONAL

#include <fn/detail/fwd.hpp>
#include <fn/detail/macro_deduced_return.hpp>
#include <fn/detail/macro_fwd.hpp>
#include <fn/detail/meta.hpp>
#include <pfn/functional.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace fn::detail {

namespace _fold_detail {
template <typename L, typename R> [[nodiscard]] constexpr auto _fold(auto &&l, auto &&r)
{
  if constexpr (_some_pack<L>) {
    return FWD(l).append(::std::in_place_type_t<R>{}, FWD(r));
  } else {
    if constexpr (_some_pack<R>) {
      return ::fn::pack<L>{FWD(l)}.append(::std::in_place_type_t<R>{}, FWD(r));
    } else {
      return ::fn::pack<L, R>{FWD(l), FWD(r)};
    }
  }
}

template <typename Lh, typename Rh>
  requires _some_sum<Lh> && _some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(lv)._transform([&rv]<typename L>(::std::in_place_type_t<L>, auto &&l) {
    return FWD(rv)._transform(
        [&l]<typename R>(::std::in_place_type_t<R>, auto &&r) { return _fold<L, R>(FWD(l), FWD(r)); });
  });
}

template <typename Lh, typename Rh>
  requires _some_sum<Lh> && (not _some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(lv)._transform(
      [&rv]<typename L>(::std::in_place_type_t<L>, auto &&l) { return _fold<L, Rh>(FWD(l), FWD(rv)); });
}

template <typename Lh, typename Rh>
  requires(not _some_sum<Lh>) && _some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return FWD(rv)._transform(
      [&lv]<typename R>(::std::in_place_type_t<R>, auto &&r) { return _fold<Lh, R>(FWD(lv), FWD(r)); });
}

template <typename Lh, typename Rh>
  requires(not _some_sum<Lh>) && (not _some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv)
{
  return _fold<Lh, Rh>(FWD(lv), FWD(rv));
}
} // namespace _fold_detail

namespace _invoke_detail {
// Each overload's noexcept is the noexcept of what it does: `std::invoke` at the bottom, the pack's
// or sum's own `invoke` when dispatching into one, and folding-then-recursing when several operands
// must be joined first (that fold constructs a pack, which can throw). The chain terminates because
// every step strictly reduces the number of pack/sum operands.
template <typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_sum<Args>))) && ::std::is_invocable_v<Fn, Args...>
[[nodiscard]] constexpr auto invoke(Fn &&fn, Args &&...args) noexcept(::std::is_nothrow_invocable_v<Fn, Args...>)
    -> DEDUCED_RETURN(::std::invoke(FWD(fn), FWD(args)...))
{
  return ::std::invoke(FWD(fn), FWD(args)...);
}

template <typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>)
          && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_sum<Args>))))
          && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).invoke(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto invoke(Fn &&fn, Arg &&arg, Args &&...args) //
    noexcept(noexcept(FWD(arg).invoke(FWD(fn), FWD(args)...))) -> DEDUCED_RETURN(FWD(arg).invoke(FWD(fn), FWD(args)...))
{
  return FWD(arg).invoke(FWD(fn), FWD(args)...);
}

template <typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_sum<Arg0>) || ... || (_some_pack<Args> || _some_sum<Args>))
          && requires(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) {
               invoke<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
                   FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
             }
// Deduced return: a trailing return type is substituted before constraints are checked, so an
// explicit one would instantiate `fold` for non-viable candidates and static_assert (a `pack` of
// rvalue refs). The body's `using type` alias is inlined only to dodge MSVC's body-local-alias leak.
[[nodiscard]] constexpr auto invoke(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) //
    noexcept(noexcept(invoke<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
        FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...))) -> decltype(auto)
{
  return invoke<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
      FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
}

template <typename Ret, typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_sum<Args>))) && ::std::is_invocable_r_v<Ret, Fn, Args...>
[[nodiscard]] constexpr auto invoke_r(Fn &&fn, Args &&...args) //
    noexcept(::std::is_nothrow_invocable_r_v<Ret, Fn, Args...>)
        -> DEDUCED_RETURN(::pfn::invoke_r<Ret>(FWD(fn), FWD(args)...))
{
  return ::pfn::invoke_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Ret, typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>)
          && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_sum<Args>))))
          && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto invoke_r(Fn &&fn, Arg &&arg, Args &&...args) //
    noexcept(noexcept(FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...)))
        -> DEDUCED_RETURN(FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...))
{
  return FWD(arg).template invoke_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Ret, typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_sum<Arg0>) || ... || (_some_pack<Args> || _some_sum<Args>))
          && requires(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) {
               invoke_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
                   FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
             }
// Same as the fold-recursing `invoke` above: deduced return, alias inlined.
[[nodiscard]] constexpr auto invoke_r(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) //
    noexcept(
        noexcept(invoke_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
            FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...))) -> decltype(auto)
{
  return invoke_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
      FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
}
} // namespace _invoke_detail

// invoke_result
template <typename Fn, typename... Args>
constexpr auto _invoke_result_result(Fn &&, Args &&...)
    -> ::std::type_identity<decltype(_invoke_detail::invoke(::std::declval<Fn>(), ::std::declval<Args>()...))>;
template <typename Fn, typename... Args> constexpr auto _invoke_result_result(auto &&...) -> ::std::type_identity<void>;

template <typename Fn, typename... Args> struct _invoke_result {
  using type = decltype(_invoke_result_result<Fn, Args...>(::std::declval<Fn>(), ::std::declval<Args>()...))::type;
};

// is_invocable
template <typename Fn, typename... Args>
constexpr auto _is_invocable_result(Fn &&, Args &&...,
                                    ::std::type_identity<decltype(::fn::detail::_invoke_detail::invoke<Fn, Args...>(
                                        ::std::declval<Fn>(), ::std::declval<Args>()...))> = {}) -> ::std::true_type;
template <typename Fn, typename... Args> constexpr auto _is_invocable_result(auto &&...) -> ::std::false_type;

template <typename Fn, typename... Args> struct _is_invocable {
  static constexpr bool value
      = decltype(_is_invocable_result<Fn, Args...>(::std::declval<Fn>(), ::std::declval<Args>()...))::value;
};

// Partial-specialization gate around `_is_invocable`: MSVC doesn't short-circuit a requires-clause
// `&&`, so a guarded `_is_invocable<Fn, sum>` conjunct gets instantiated even when an earlier guard
// is already false, tripping sum's uniform-result static_assert; selecting the false_type primary
// keeps the probe out of its reach (no-op on compilers that do short-circuit).
template <bool Enable, typename Fn, typename... Args> struct _is_invocable_if : ::std::false_type {};
template <typename Fn, typename... Args> struct _is_invocable_if<true, Fn, Args...> : _is_invocable<Fn, Args...> {};

// is_invocable_r
template <typename Ret, typename Fn, typename... Args>
constexpr auto _is_invocable_r_result(
    Fn &&, Args &&...,
    ::std::type_identity<decltype(_invoke_detail::invoke_r<Ret>(::std::declval<Fn>(), ::std::declval<Args>()...))> = {})
    -> ::std::true_type;
template <typename Ret, typename Fn, typename... Args>
constexpr auto _is_invocable_r_result(auto &&...) -> ::std::false_type;
template <typename Ret, typename Fn, typename... Args> struct _is_invocable_r {
  static constexpr bool value
      = decltype(_is_invocable_r_result<Ret, Fn, Args...>(::std::declval<Fn>(), ::std::declval<Args>()...))::value;
};

// is_nothrow_invocable and is_nothrow_invocable_v. The invoke chain above carries its own spec, so
// the question is asked of the call itself and composes through pack and sum dispatch: the answer
// for a sum operand is that every alternative's call is nothrow, and for a pack that the call over
// its elements is. The bool parameter keeps the noexcept operand out of reach when the call is not
// viable at all, where it would be ill-formed rather than false.
template <bool Enable, typename Fn, typename... Args> struct _is_nothrow_invocable_impl : ::std::false_type {};
template <typename Fn, typename... Args>
struct _is_nothrow_invocable_impl<true, Fn, Args...>
    : ::std::bool_constant<noexcept(_invoke_detail::invoke(::std::declval<Fn>(), ::std::declval<Args>()...))> {};

template <typename Fn, typename... Args>
struct _is_nothrow_invocable : _is_nothrow_invocable_impl<_is_invocable<Fn, Args...>::value, Fn, Args...> {};

// is_nothrow_invocable_r and is_nothrow_invocable_r_v
template <bool Enable, typename Ret, typename Fn, typename... Args>
struct _is_nothrow_invocable_r_impl : ::std::false_type {};
template <typename Ret, typename Fn, typename... Args>
struct _is_nothrow_invocable_r_impl<true, Ret, Fn, Args...>
    : ::std::bool_constant<noexcept(_invoke_detail::invoke_r<Ret>(::std::declval<Fn>(), ::std::declval<Args>()...))> {};

template <typename Ret, typename Fn, typename... Args>
struct _is_nothrow_invocable_r
    : _is_nothrow_invocable_r_impl<_is_invocable_r<Ret, Fn, Args...>::value, Ret, Fn, Args...> {};

// invoke
template <typename Fn, typename... Args>
  requires(_is_invocable<Fn, Args...>::value)
constexpr auto _invoke(Fn &&fn, Args &&...args) noexcept(_is_nothrow_invocable<Fn, Args...>::value)
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

// Nothrow twins of the two folds above: a dispatch over a typelist can throw unless every
// alternative's call is nothrow, since which one runs is not known until run time.
template <typename Fn, typename T, typename... Tx> constexpr inline bool _is_nothrow_ts_invocable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_invocable<Fn, Tpl<Ts...> &, Tx...>
    = (... && _is_nothrow_invocable<Fn, Ts &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_invocable<Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_nothrow_invocable<Fn, Ts const &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_invocable<Fn, Tpl<Ts...> &&, Tx...>
    = (... && _is_nothrow_invocable<Fn, Ts &&, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_invocable<Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_nothrow_invocable<Fn, Ts const &&, Tx...>::value);
template <typename Fn, typename T, typename... Tx>
concept _typelist_nothrow_invocable = _is_nothrow_ts_invocable<Fn, T &&, Tx...>;

template <typename R, typename Fn, typename T, typename... Tx> constexpr inline bool _is_nothrow_rts_invocable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_invocable<R, Fn, Tpl<Ts...> &, Tx...>
    = (... && _is_nothrow_invocable_r<R, Fn, Ts &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_invocable<R, Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_nothrow_invocable_r<R, Fn, Ts const &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_invocable<R, Fn, Tpl<Ts...> &&, Tx...>
    = (... && _is_nothrow_invocable_r<R, Fn, Ts &&, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_invocable<R, Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_nothrow_invocable_r<R, Fn, Ts const &&, Tx...>::value);
template <typename R, typename Fn, typename T, typename... Tx>
concept _typelist_nothrow_invocable_r = _is_nothrow_rts_invocable<R, Fn, T &&, Tx...>;

} // namespace fn::detail

#endif // INCLUDE_FN_DETAIL_FUNCTIONAL
