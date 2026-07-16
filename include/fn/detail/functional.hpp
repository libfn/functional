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
#include <pfn/tuple.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace fn::detail {

namespace _fold_detail {
// The branch the fold takes is chosen by `if constexpr`, so a single expression cannot state its
// specification: the two untaken spellings would be ill-formed. Hence one specialization per branch.
template <typename L, typename R, typename Lv, typename Rv>
struct _nothrow_fold : ::std::bool_constant<noexcept(::fn::pack<L, R>{::std::declval<Lv>(), ::std::declval<Rv>()})> {};
template <typename L, typename R, typename Lv, typename Rv>
  requires _some_pack<L>
struct _nothrow_fold<L, R, Lv, Rv>
    : ::std::bool_constant<noexcept(::std::declval<Lv>().append(::std::in_place_type_t<R>{}, ::std::declval<Rv>()))> {};
template <typename L, typename R, typename Lv, typename Rv>
  requires(not _some_pack<L>) && _some_pack<R>
struct _nothrow_fold<L, R, Lv, Rv> : ::std::bool_constant<noexcept(::fn::pack<L>{::std::declval<Lv>()}.append(
                                         ::std::in_place_type_t<R>{}, ::std::declval<Rv>()))> {};

template <typename L, typename R>
[[nodiscard]] constexpr auto _fold(auto &&l, auto &&r) //
    noexcept(_nothrow_fold<L, R, decltype(l), decltype(r)>::value)
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

// Named types, not lambdas: `fold` and everything above it specify themselves in terms of what
// dispatching through these promises, and a lambda can be named neither in a noexcept-specifier nor
// (before clang 17) in any unevaluated operand at all.
template <typename R, typename Rv> struct _fold_rh final {
  Rv rv;

  template <typename L>
  [[nodiscard]] constexpr auto operator()(::std::in_place_type_t<L>, auto &&l) const
      noexcept(noexcept(_fold<L, R>(FWD(l), ::std::declval<Rv>())))
  {
    return _fold<L, R>(FWD(l), static_cast<Rv &&>(rv));
  }
};

template <typename L, typename Lv> struct _fold_lh final {
  Lv lv;

  template <typename R>
  [[nodiscard]] constexpr auto operator()(::std::in_place_type_t<R>, auto &&r) const
      noexcept(noexcept(_fold<L, R>(::std::declval<Lv>(), FWD(r))))
  {
    return _fold<L, R>(static_cast<Lv &&>(lv), FWD(r));
  }
};

template <typename Rv> struct _fold_rh_sum final {
  Rv rv;

  template <typename L>
  [[nodiscard]] constexpr auto operator()(::std::in_place_type_t<L>, auto &&l) const
      noexcept(noexcept(::std::declval<Rv>()._transform(_fold_lh<L, decltype(l)>{FWD(l)})))
  {
    return static_cast<Rv &&>(rv)._transform(_fold_lh<L, decltype(l)>{FWD(l)});
  }
};

template <typename Lh, typename Rh>
  requires _some_sum<Lh> && _some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(FWD(lv)._transform(_fold_rh_sum<decltype(rv)>{FWD(rv)})))
{
  return FWD(lv)._transform(_fold_rh_sum<decltype(rv)>{FWD(rv)});
}

template <typename Lh, typename Rh>
  requires _some_sum<Lh> && (not _some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(FWD(lv)._transform(_fold_rh<Rh, decltype(rv)>{FWD(rv)})))
{
  return FWD(lv)._transform(_fold_rh<Rh, decltype(rv)>{FWD(rv)});
}

template <typename Lh, typename Rh>
  requires(not _some_sum<Lh>) && _some_sum<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(FWD(rv)._transform(_fold_lh<Lh, decltype(lv)>{FWD(lv)})))
{
  return FWD(rv)._transform(_fold_lh<Lh, decltype(lv)>{FWD(lv)});
}

template <typename Lh, typename Rh>
  requires(not _some_sum<Lh>) && (not _some_sum<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(_fold<Lh, Rh>(FWD(lv), FWD(rv))))
{
  return _fold<Lh, Rh>(FWD(lv), FWD(rv));
}
} // namespace _fold_detail

namespace _apply_detail {
// Each overload's noexcept is the noexcept of what it does: `std::invoke` at the bottom, the pack's
// or sum's own `apply` when dispatching into one, and folding-then-recursing when several operands
// must be joined first (that fold constructs a pack, which can throw). The chain terminates because
// every step strictly reduces the number of pack/sum operands.
template <typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_sum<Args>))) && ::std::is_invocable_v<Fn, Args...>
[[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) noexcept(::std::is_nothrow_invocable_v<Fn, Args...>)
    -> DEDUCED_RETURN(::std::invoke(FWD(fn), FWD(args)...))
{
  return ::std::invoke(FWD(fn), FWD(args)...);
}

// A lone tuple-like argument takes std::apply's meaning, through the machinery shared with
// pfn::apply - so fn::apply and pfn::apply agree on the entire std domain by construction, and
// the elements are terminal: they do not re-enter pack or sum dispatch (a sum element is handed
// over whole). Exactly std::apply's shape - one tuple-like argument, nothing else; composition
// with packs, sums or further arguments is not offered. Where both this and the pass-whole
// terminal above are viable (a generic callable), this non-variadic overload wins partial
// ordering: std::apply's meaning prevails on std::apply's own domain.
template <typename Fn, typename Arg>
  requires ::pfn::detail::_tuple_like<Arg> && (::pfn::is_applicable_v<Fn, Arg>)
[[nodiscard]] constexpr auto apply(Fn &&fn, Arg &&arg) noexcept(::pfn::is_nothrow_applicable_v<Fn, Arg>)
    -> ::pfn::apply_result_t<Fn, Arg>
{
  return ::pfn::apply(FWD(fn), FWD(arg));
}

template <typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>)
          && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_sum<Args>))))
          && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).apply(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto apply(Fn &&fn, Arg &&arg, Args &&...args) //
    noexcept(noexcept(FWD(arg).apply(FWD(fn), FWD(args)...))) -> DEDUCED_RETURN(FWD(arg).apply(FWD(fn), FWD(args)...))
{
  return FWD(arg).apply(FWD(fn), FWD(args)...);
}

template <typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_sum<Arg0>) || ... || (_some_pack<Args> || _some_sum<Args>))
          && requires(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) {
               apply<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
                   FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
             }
// Deduced return: a trailing return type is substituted before constraints are checked, so an
// explicit one would instantiate `fold` for non-viable candidates and static_assert (a `pack` of
// rvalue refs). The body's `using type` alias is inlined only to dodge MSVC's body-local-alias leak.
[[nodiscard]] constexpr auto apply(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) //
    noexcept(noexcept(apply<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
        FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...))) -> decltype(auto)
{
  return apply<Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
      FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
}

template <typename Ret, typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_sum<Args>))) && ::std::is_invocable_r_v<Ret, Fn, Args...>
[[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) //
    noexcept(::std::is_nothrow_invocable_r_v<Ret, Fn, Args...>)
        -> DEDUCED_RETURN(::pfn::invoke_r<Ret>(FWD(fn), FWD(args)...))
{
  return ::pfn::invoke_r<Ret>(FWD(fn), FWD(args)...);
}

// The tuple-like arm of apply_r, mirroring apply's: INVOKE<R> over the elements. Constrained so
// a non-viable call is a substitution failure before the noexcept-specifier can instantiate.
template <typename Ret, typename Fn, typename Tuple, ::std::size_t... Ix>
  requires ::std::is_invocable_r_v<Ret, Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))...>
constexpr Ret _apply_r_elems(Fn &&fn, Tuple &&t, ::std::index_sequence<Ix...>) //
    noexcept(noexcept(::pfn::invoke_r<Ret>(FWD(fn), ::std::get<Ix>(FWD(t))...)))
{
  return ::pfn::invoke_r<Ret>(FWD(fn), ::std::get<Ix>(FWD(t))...);
}

template <typename Ret, typename Fn, typename Arg>
  requires ::pfn::detail::_tuple_like<Arg> //
           && requires(Fn &&fn, Arg &&arg) {
                _apply_r_elems<Ret>(FWD(fn), FWD(arg),
                                    ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Arg>>>{});
              }
[[nodiscard]] constexpr auto apply_r(Fn &&fn, Arg &&arg) //
    noexcept(noexcept(_apply_r_elems<Ret>(
        FWD(fn), FWD(arg), ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Arg>>>{}))) -> Ret
{
  return _apply_r_elems<Ret>(FWD(fn), FWD(arg),
                             ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Arg>>>{});
}

template <typename Ret, typename Fn, typename Arg, typename... Args>
  requires(_some_pack<Arg> || _some_sum<Arg>)
          && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_sum<Args>))))
          && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto apply_r(Fn &&fn, Arg &&arg, Args &&...args) //
    noexcept(noexcept(FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...)))
        -> DEDUCED_RETURN(FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...))
{
  return FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Ret, typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_sum<Arg0>) || ... || (_some_pack<Args> || _some_sum<Args>))
          && requires(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) {
               apply_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
                   FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
             }
// Same as the fold-recursing `apply` above: deduced return, alias inlined.
[[nodiscard]] constexpr auto apply_r(Fn &&fn, Arg &&arg, Arg0 &&arg0, Args &&...args) //
    noexcept(
        noexcept(apply_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
            FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...))) -> decltype(auto)
{
  return apply_r<Ret, Fn, decltype(::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0))), Args...>(
      FWD(fn), ::fn::detail::_fold_detail::fold<Arg, Arg0>(FWD(arg), FWD(arg0)), FWD(args)...);
}
} // namespace _apply_detail

// apply_result
template <typename Fn, typename... Args>
constexpr auto _apply_result_result(Fn &&, Args &&...)
    -> ::std::type_identity<decltype(_apply_detail::apply(::std::declval<Fn>(), ::std::declval<Args>()...))>;
template <typename Fn, typename... Args> constexpr auto _apply_result_result(auto &&...) -> ::std::type_identity<void>;

template <typename Fn, typename... Args> struct _apply_result {
  using type = decltype(_apply_result_result<Fn, Args...>(::std::declval<Fn>(), ::std::declval<Args>()...))::type;
};

// is_applicable
template <typename Fn, typename... Args>
constexpr auto _is_applicable_result(Fn &&, Args &&...,
                                     ::std::type_identity<decltype(::fn::detail::_apply_detail::apply<Fn, Args...>(
                                         ::std::declval<Fn>(), ::std::declval<Args>()...))> = {}) -> ::std::true_type;
template <typename Fn, typename... Args> constexpr auto _is_applicable_result(auto &&...) -> ::std::false_type;

template <typename Fn, typename... Args> struct _is_applicable {
  static constexpr bool value
      = decltype(_is_applicable_result<Fn, Args...>(::std::declval<Fn>(), ::std::declval<Args>()...))::value;
};

// Partial-specialization gate around `_is_applicable`: MSVC doesn't short-circuit a requires-clause
// `&&`, so a guarded `_is_applicable<Fn, sum>` conjunct gets instantiated even when an earlier guard
// is already false, tripping sum's uniform-result static_assert; selecting the false_type primary
// keeps the probe out of its reach (no-op on compilers that do short-circuit).
template <bool Enable, typename Fn, typename... Args> struct _is_applicable_if : ::std::false_type {};
template <typename Fn, typename... Args> struct _is_applicable_if<true, Fn, Args...> : _is_applicable<Fn, Args...> {};

// is_applicable_r
template <typename Ret, typename Fn, typename... Args>
constexpr auto _is_applicable_r_result(
    Fn &&, Args &&...,
    ::std::type_identity<decltype(_apply_detail::apply_r<Ret>(::std::declval<Fn>(), ::std::declval<Args>()...))> = {})
    -> ::std::true_type;
template <typename Ret, typename Fn, typename... Args>
constexpr auto _is_applicable_r_result(auto &&...) -> ::std::false_type;
template <typename Ret, typename Fn, typename... Args> struct _is_applicable_r {
  static constexpr bool value
      = decltype(_is_applicable_r_result<Ret, Fn, Args...>(::std::declval<Fn>(), ::std::declval<Args>()...))::value;
};

// is_nothrow_applicable and is_nothrow_applicable_v. The apply chain above carries its own spec, so
// the question is asked of the call itself and composes through pack and sum dispatch: the answer
// for a sum operand is that every alternative's call is nothrow, and for a pack that the call over
// its elements is. The bool parameter keeps the noexcept operand out of reach when the call is not
// viable at all, where it would be ill-formed rather than false.
template <bool Enable, typename Fn, typename... Args> struct _is_nothrow_applicable_impl : ::std::false_type {};
template <typename Fn, typename... Args>
struct _is_nothrow_applicable_impl<true, Fn, Args...>
    : ::std::bool_constant<noexcept(_apply_detail::apply(::std::declval<Fn>(), ::std::declval<Args>()...))> {};

template <typename Fn, typename... Args>
struct _is_nothrow_applicable : _is_nothrow_applicable_impl<_is_applicable<Fn, Args...>::value, Fn, Args...> {};

// is_nothrow_applicable_r and is_nothrow_applicable_r_v
template <bool Enable, typename Ret, typename Fn, typename... Args>
struct _is_nothrow_applicable_r_impl : ::std::false_type {};
template <typename Ret, typename Fn, typename... Args>
struct _is_nothrow_applicable_r_impl<true, Ret, Fn, Args...>
    : ::std::bool_constant<noexcept(_apply_detail::apply_r<Ret>(::std::declval<Fn>(), ::std::declval<Args>()...))> {};

template <typename Ret, typename Fn, typename... Args>
struct _is_nothrow_applicable_r
    : _is_nothrow_applicable_r_impl<_is_applicable_r<Ret, Fn, Args...>::value, Ret, Fn, Args...> {};

// apply
template <typename Fn, typename... Args>
  requires(_is_applicable<Fn, Args...>::value)
constexpr auto _apply(Fn &&fn, Args &&...args) noexcept(_is_nothrow_applicable<Fn, Args...>::value)
    -> _apply_result<Fn, Args...>::type
{
  return _apply_detail::apply(FWD(fn), FWD(args)...);
}

// apply_r
template <typename Ret, typename Fn, typename... Args>
  requires(_is_applicable_r<Ret, Fn, Args...>::value)
constexpr auto _apply_r(Fn &&fn, Args &&...args) noexcept(_is_nothrow_applicable_r<Ret, Fn, Args...>::value) -> Ret
{
  return _apply_detail::apply_r<Ret>(FWD(fn), FWD(args)...);
}

// Named (a lambda cannot appear in a noexcept-specifier): prepends the alternative's tag to the
// unpacked elements, so one shape serves both the pack and the tuple-like unpacking below.
template <typename Fn, typename T> struct _apply_type_elems final {
  Fn &&fn;

  template <typename... Args>
    requires ::std::is_invocable_v<Fn, ::std::in_place_type_t<T>, Args...>
  constexpr auto operator()(Args &&...args) && //
      noexcept(::std::is_nothrow_invocable_v<Fn, ::std::in_place_type_t<T>, Args...>)
          -> DEDUCED_RETURN(::std::invoke(FWD(fn), ::std::in_place_type_t<T>{}, FWD(args)...))
  {
    return ::std::invoke(FWD(fn), ::std::in_place_type_t<T>{}, FWD(args)...);
  }
};

// The apply_type arm adapter: the type-indexed dispatch hands (tag, whole value); this re-invokes
// the user's arm set with the alternative unpacked exactly as value-path apply would unpack it.
template <typename Fn> struct _apply_type_fn final {
  Fn &&fn;

  template <typename T, typename V>
  constexpr auto operator()(::std::in_place_type_t<T>, V &&v) && //
      noexcept(noexcept(::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)})))
          -> DEDUCED_RETURN(::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)}))
    requires _some_pack<T>
             && requires { ::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)}); }
  {
    return ::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)});
  }

  template <typename T, typename V>
  constexpr auto operator()(::std::in_place_type_t<T>, V &&v) && //
      noexcept(::pfn::is_nothrow_applicable_v<_apply_type_elems<Fn, T>, V>)
          -> DEDUCED_RETURN(::pfn::apply(_apply_type_elems<Fn, T>{FWD(fn)}, FWD(v)))
    requires(not _some_pack<T>)
            && ::pfn::detail::_tuple_like<T> && (::pfn::is_applicable_v<_apply_type_elems<Fn, T>, V>)
  {
    return ::pfn::apply(_apply_type_elems<Fn, T>{FWD(fn)}, FWD(v));
  }

  template <typename T, typename V>
  constexpr auto operator()(::std::in_place_type_t<T> tag, V &&v) && //
      noexcept(::std::is_nothrow_invocable_v<Fn, ::std::in_place_type_t<T>, V>)
          -> DEDUCED_RETURN(::std::invoke(FWD(fn), tag, FWD(v)))
    requires(not _some_pack<T>) && (not ::pfn::detail::_tuple_like<T>)
            && ::std::is_invocable_v<Fn, ::std::in_place_type_t<T>, V>
  {
    return ::std::invoke(FWD(fn), tag, FWD(v));
  }
};

template <typename Fn, typename T, typename... Tx> constexpr inline bool _is_ts_applicable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_applicable<Fn, Tpl<Ts...> &, Tx...> = (... && _is_applicable<Fn, Ts &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_applicable<Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_applicable<Fn, Ts const &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_applicable<Fn, Tpl<Ts...> &&, Tx...> = (... && _is_applicable<Fn, Ts &&, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_ts_applicable<Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_applicable<Fn, Ts const &&, Tx...>::value);
template <typename Fn, typename T, typename... Tx>
concept _typelist_applicable = _is_ts_applicable<Fn, T &&, Tx...>;

template <typename R, typename Fn, typename T, typename... Tx> constexpr inline bool _is_rts_applicable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_applicable<R, Fn, Tpl<Ts...> &, Tx...>
    = (... && _is_applicable_r<R, Fn, Ts &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_applicable<R, Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_applicable_r<R, Fn, Ts const &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_applicable<R, Fn, Tpl<Ts...> &&, Tx...>
    = (... && _is_applicable_r<R, Fn, Ts &&, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_rts_applicable<R, Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_applicable_r<R, Fn, Ts const &&, Tx...>::value);
template <typename R, typename Fn, typename T, typename... Tx>
concept _typelist_applicable_r = _is_rts_applicable<R, Fn, T &&, Tx...>;

// Nothrow twins of the two folds above: a dispatch over a typelist can throw unless every
// alternative's call is nothrow, since which one runs is not known until run time.
template <typename Fn, typename T, typename... Tx> constexpr inline bool _is_nothrow_ts_applicable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_applicable<Fn, Tpl<Ts...> &, Tx...>
    = (... && _is_nothrow_applicable<Fn, Ts &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_applicable<Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_nothrow_applicable<Fn, Ts const &, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_applicable<Fn, Tpl<Ts...> &&, Tx...>
    = (... && _is_nothrow_applicable<Fn, Ts &&, Tx...>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_ts_applicable<Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_nothrow_applicable<Fn, Ts const &&, Tx...>::value);
template <typename Fn, typename T, typename... Tx>
concept _typelist_nothrow_applicable = _is_nothrow_ts_applicable<Fn, T &&, Tx...>;

template <typename R, typename Fn, typename T, typename... Tx> constexpr inline bool _is_nothrow_rts_applicable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_applicable<R, Fn, Tpl<Ts...> &, Tx...>
    = (... && _is_nothrow_applicable_r<R, Fn, Ts &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_applicable<R, Fn, Tpl<Ts...> const &, Tx...>
    = (... && _is_nothrow_applicable_r<R, Fn, Ts const &, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_applicable<R, Fn, Tpl<Ts...> &&, Tx...>
    = (... && _is_nothrow_applicable_r<R, Fn, Ts &&, Tx...>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts, typename... Tx>
constexpr inline bool _is_nothrow_rts_applicable<R, Fn, Tpl<Ts...> const &&, Tx...>
    = (... && _is_nothrow_applicable_r<R, Fn, Ts const &&, Tx...>::value);
template <typename R, typename Fn, typename T, typename... Tx>
concept _typelist_nothrow_applicable_r = _is_nothrow_rts_applicable<R, Fn, T &&, Tx...>;

} // namespace fn::detail

#endif // INCLUDE_FN_DETAIL_FUNCTIONAL
