// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_FUNCTIONAL
#define INCLUDE_FN_DETAIL_FUNCTIONAL

#include <fn/detail/fwd.hpp>
#include <fn/detail/meta.hpp>
#include <pfn/functional.hpp>
#include <pfn/tuple.hpp>

#include <functional>
#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

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

template <typename Rv> struct _fold_rh_copack final {
  Rv rv;

  template <typename L>
  [[nodiscard]] constexpr auto operator()(::std::in_place_type_t<L>, auto &&l) const
      noexcept(noexcept(::std::declval<Rv>()._transform(_fold_lh<L, decltype(l)>{FWD(l)})))
  {
    return static_cast<Rv &&>(rv)._transform(_fold_lh<L, decltype(l)>{FWD(l)});
  }
};

template <typename Lh, typename Rh>
  requires _some_copack<Lh> && _some_copack<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(FWD(lv)._transform(_fold_rh_copack<decltype(rv)>{FWD(rv)})))
{
  return FWD(lv)._transform(_fold_rh_copack<decltype(rv)>{FWD(rv)});
}

template <typename Lh, typename Rh>
  requires _some_copack<Lh> && (not _some_copack<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(FWD(lv)._transform(_fold_rh<Rh, decltype(rv)>{FWD(rv)})))
{
  return FWD(lv)._transform(_fold_rh<Rh, decltype(rv)>{FWD(rv)});
}

template <typename Lh, typename Rh>
  requires(not _some_copack<Lh>) && _some_copack<Rh>
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(FWD(rv)._transform(_fold_lh<Lh, decltype(lv)>{FWD(lv)})))
{
  return FWD(rv)._transform(_fold_lh<Lh, decltype(lv)>{FWD(lv)});
}

template <typename Lh, typename Rh>
  requires(not _some_copack<Lh>) && (not _some_copack<Rh>)
[[nodiscard]] constexpr auto fold(auto &&lv, auto &&rv) //
    noexcept(noexcept(_fold<Lh, Rh>(FWD(lv), FWD(rv))))
{
  return _fold<Lh, Rh>(FWD(lv), FWD(rv));
}
} // namespace _fold_detail

namespace _apply_detail {
// Each overload's noexcept is the noexcept of what it does: `std::invoke` at the bottom, the pack's
// or copack's own `apply` when dispatching into one, and folding-then-recursing when several operands
// must be joined first (that fold constructs a pack, which can throw). The chain terminates because
// every step strictly reduces the number of pack/copack operands.
template <typename Fn, typename... Args>
  requires(not(... || (_some_pack<Args> || _some_copack<Args>))) && ::std::is_invocable_v<Fn, Args...>
[[nodiscard]] constexpr auto apply(Fn &&fn, Args &&...args) noexcept(::std::is_nothrow_invocable_v<Fn, Args...>)
    -> DEDUCED_RETURN(::std::invoke(FWD(fn), FWD(args)...))
{
  return ::std::invoke(FWD(fn), FWD(args)...);
}

// A lone tuple-like argument takes std::apply's meaning, through the machinery shared with
// pfn::apply - so fn::apply and pfn::apply agree on the entire std domain by construction, and
// the elements are terminal: they do not re-enter pack or copack dispatch (a copack element is handed
// over whole). Exactly std::apply's shape - one tuple-like argument, nothing else; composition
// with packs, copacks or further arguments is not offered. Where both this and the pass-whole
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
  requires(_some_pack<Arg> || _some_copack<Arg>)
          && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_copack<Args>))))
          && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).apply(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto apply(Fn &&fn, Arg &&arg, Args &&...args) //
    noexcept(noexcept(FWD(arg).apply(FWD(fn), FWD(args)...))) -> DEDUCED_RETURN(FWD(arg).apply(FWD(fn), FWD(args)...))
{
  return FWD(arg).apply(FWD(fn), FWD(args)...);
}

template <typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_copack<Arg0>) || ... || (_some_pack<Args> || _some_copack<Args>))
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
  requires(not(... || (_some_pack<Args> || _some_copack<Args>))) && ::std::is_invocable_r_v<Ret, Fn, Args...>
[[nodiscard]] constexpr auto apply_r(Fn &&fn, Args &&...args) //
    noexcept(::std::is_nothrow_invocable_r_v<Ret, Fn, Args...>)
        -> DEDUCED_RETURN(::pfn::invoke_r<Ret>(FWD(fn), FWD(args)...))
{
  return ::pfn::invoke_r<Ret>(FWD(fn), FWD(args)...);
}

// The tuple-like arm of apply_r, mirroring apply's: INVOKE<R> over the elements, followed by any
// trailing arguments. Constrained so a non-viable call is a substitution failure before the
// noexcept-specifier can instantiate.
template <typename Ret, typename Fn, typename Tuple, ::std::size_t... Ix, typename... Args>
  requires ::std::is_invocable_r_v<Ret, Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>
constexpr Ret _apply_r_elems(Fn &&fn, Tuple &&t, ::std::index_sequence<Ix...>, Args &&...args) //
    noexcept(noexcept(::pfn::invoke_r<Ret>(FWD(fn), ::std::get<Ix>(FWD(t))..., FWD(args)...)))
{
  return ::pfn::invoke_r<Ret>(FWD(fn), ::std::get<Ix>(FWD(t))..., FWD(args)...);
}

// The INVOKE twin, serving the tagged dispatchers' tuple-like arms below.
template <typename Fn, typename Tuple, ::std::size_t... Ix, typename... Args>
  requires ::std::is_invocable_v<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>
constexpr auto _apply_elems(Fn &&fn, Tuple &&t, ::std::index_sequence<Ix...>, Args &&...args) //
    noexcept(::std::is_nothrow_invocable_v<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>)
        -> DEDUCED_RETURN(::std::invoke(FWD(fn), ::std::get<Ix>(FWD(t))..., FWD(args)...))
{
  return ::std::invoke(FWD(fn), ::std::get<Ix>(FWD(t))..., FWD(args)...);
}

// SFINAE-friendly elements-and-trailing-arguments traits, gated like pfn's _apply_traits: the
// tuple-like arms below must not name tuple_size raw in their declarations, which MSVC
// substitutes eagerly even for a constrained-out candidate - through the gate a non-tuple
// subject answers false (and yields no result type) instead of hard-erroring.
template <typename Fn, typename Tuple, typename Ix, typename... Args> struct _elems_probe;
template <typename Fn, typename Tuple, ::std::size_t... Ix, typename... Args>
struct _elems_probe<Fn, Tuple, ::std::index_sequence<Ix...>, Args...> {
  static constexpr bool _invocable
      = ::std::is_invocable_v<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>;
  static constexpr bool _nothrow
      = ::std::is_nothrow_invocable_v<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>;
  using _result = ::std::invoke_result<Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>;
};

template <bool, typename Fn, typename Tuple, typename... Args> struct _elems_gate { // not tuple-like
  static constexpr bool _invocable = false;
  static constexpr bool _nothrow = false;
  struct _result {}; // no member `type`, [meta.trans.other]
};
template <typename Fn, typename Tuple, typename... Args>
struct _elems_gate<true, Fn, Tuple, Args...>
    : _elems_probe<Fn, Tuple, ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Tuple>>>,
                   Args...> {};

template <typename Fn, typename Tuple, typename... Args>
using _elems_traits
    = _elems_gate<::pfn::detail::_tuple_like<Tuple> && ::pfn::detail::_tuple_sized<Tuple>, Fn, Tuple, Args...>;

template <typename Ret, typename Fn, typename Tuple, typename Ix, typename... Args> struct _elems_probe_r;
template <typename Ret, typename Fn, typename Tuple, ::std::size_t... Ix, typename... Args>
struct _elems_probe_r<Ret, Fn, Tuple, ::std::index_sequence<Ix...>, Args...> {
  static constexpr bool _invocable
      = ::std::is_invocable_r_v<Ret, Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>;
  static constexpr bool _nothrow
      = ::std::is_nothrow_invocable_r_v<Ret, Fn, decltype(::std::get<Ix>(::std::declval<Tuple>()))..., Args...>;
};

template <bool, typename Ret, typename Fn, typename Tuple, typename... Args> struct _elems_gate_r {
  static constexpr bool _invocable = false;
  static constexpr bool _nothrow = false;
};
template <typename Ret, typename Fn, typename Tuple, typename... Args>
struct _elems_gate_r<true, Ret, Fn, Tuple, Args...>
    : _elems_probe_r<Ret, Fn, Tuple, ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<Tuple>>>,
                     Args...> {};

template <typename Ret, typename Fn, typename Tuple, typename... Args>
using _elems_traits_r
    = _elems_gate_r<::pfn::detail::_tuple_like<Tuple> && ::pfn::detail::_tuple_sized<Tuple>, Ret, Fn, Tuple, Args...>;

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
  requires(_some_pack<Arg> || _some_copack<Arg>)
          && ((sizeof...(Args) == 0) || (not(... || (_some_pack<Args> || _some_copack<Args>))))
          && requires(Fn &&fn, Arg &&arg, Args &&...args) { FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...); }
[[nodiscard]] constexpr auto apply_r(Fn &&fn, Arg &&arg, Args &&...args) //
    noexcept(noexcept(FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...)))
        -> DEDUCED_RETURN(FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...))
{
  return FWD(arg).template apply_r<Ret>(FWD(fn), FWD(args)...);
}

template <typename Ret, typename Fn, typename Arg, typename Arg0, typename... Args>
  requires((_some_pack<Arg0> || _some_copack<Arg0>) || ... || (_some_pack<Args> || _some_copack<Args>))
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
// `&&`, so a guarded `_is_applicable<Fn, copack>` conjunct gets instantiated even when an earlier guard
// is already false, tripping copack's uniform-result static_assert; selecting the false_type primary
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
// the question is asked of the call itself and composes through pack and copack dispatch: the answer
// for a copack operand is that every alternative's call is nothrow, and for a pack that the call over
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

// Named (a lambda cannot appear in a noexcept-specifier): prepends a default-constructed tag to
// the unpacked elements, one shape for every tagged surface - copack/choice apply_type below, and
// the tagged members of optional and expected.
template <typename Fn, typename Tag> struct _apply_tag_elems final {
  Fn &&fn;

  template <typename... Args>
    requires ::std::is_invocable_v<Fn, Tag, Args...>
  constexpr auto operator()(Args &&...args) && //
      noexcept(::std::is_nothrow_invocable_v<Fn, Tag, Args...>)
          -> DEDUCED_RETURN(::std::invoke(FWD(fn), Tag{}, FWD(args)...))
  {
    return ::std::invoke(FWD(fn), Tag{}, FWD(args)...);
  }
};

template <typename Fn, typename T> using _apply_type_elems = _apply_tag_elems<Fn, ::std::in_place_type_t<T>>;

// The tagged elimination of one value - the engaged/error arm of optional's and expected's
// apply_type. The tag is prepended to the value unpacked as _apply would unpack it (trailing
// arguments follow the elements), except that a tuple-like value's elements form is the row's one
// signature (no pass-whole fallback), as on copack::apply_type; a pack or copack consumes through its
// own member apply, and anything else - including a choice, a nominal boundary - is handed over
// whole.
template <typename Tag, typename Fn, typename V, typename... Args>
  requires(_some_pack<V> || _some_copack<V>) && requires(Fn &&fn, V &&v, Args &&...args) {
    FWD(v).apply(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...);
  }
[[nodiscard]] constexpr auto _apply_tagged(Fn &&fn, V &&v, Args &&...args) //
    noexcept(noexcept(FWD(v).apply(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...)))
        -> DEDUCED_RETURN(FWD(v).apply(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...))
{
  return FWD(v).apply(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...);
}

template <typename Tag, typename Fn, typename V, typename... Args>
  requires(not _some_pack<V>) && (not _some_copack<V>) && ::pfn::detail::_tuple_like<V>
          && (_apply_detail::_elems_traits<_apply_tag_elems<Fn, Tag>, V, Args...>::_invocable)
[[nodiscard]] constexpr auto _apply_tagged(Fn &&fn, V &&v, Args &&...args) //
    noexcept(_apply_detail::_elems_traits<_apply_tag_elems<Fn, Tag>, V, Args...>::_nothrow) ->
    typename _apply_detail::_elems_traits<_apply_tag_elems<Fn, Tag>, V, Args...>::_result::type
{
  return _apply_detail::_apply_elems(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(v),
                                     ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<V>>>{},
                                     FWD(args)...);
}

// The tag is passed as a prvalue, the exact shape the traits above ask about (a named parameter
// would be an lvalue, splitting the probe from the deed).
template <typename Tag, typename Fn, typename V, typename... Args>
  requires(not _some_pack<V>) && (not _some_copack<V>) && (not ::pfn::detail::_tuple_like<V>)
          && ::std::is_invocable_v<Fn, Tag, V, Args...>
[[nodiscard]] constexpr auto _apply_tagged(Fn &&fn, V &&v, Args &&...args) //
    noexcept(::std::is_nothrow_invocable_v<Fn, Tag, V, Args...>)
        -> DEDUCED_RETURN(::std::invoke(FWD(fn), Tag{}, FWD(v), FWD(args)...))
{
  return ::std::invoke(FWD(fn), Tag{}, FWD(v), FWD(args)...);
}

template <typename Ret, typename Tag, typename Fn, typename V, typename... Args>
  requires(_some_pack<V> || _some_copack<V>) && requires(Fn &&fn, V &&v, Args &&...args) {
    FWD(v).template apply_r<Ret>(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...);
  }
[[nodiscard]] constexpr auto _apply_tagged_r(Fn &&fn, V &&v, Args &&...args) //
    noexcept(noexcept(FWD(v).template apply_r<Ret>(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...))) -> Ret
{
  return FWD(v).template apply_r<Ret>(_apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(args)...);
}

template <typename Ret, typename Tag, typename Fn, typename V, typename... Args>
  requires(not _some_pack<V>) && (not _some_copack<V>) && ::pfn::detail::_tuple_like<V>
          && (_apply_detail::_elems_traits_r<Ret, _apply_tag_elems<Fn, Tag>, V, Args...>::_invocable)
[[nodiscard]] constexpr auto _apply_tagged_r(Fn &&fn, V &&v, Args &&...args) //
    noexcept(_apply_detail::_elems_traits_r<Ret, _apply_tag_elems<Fn, Tag>, V, Args...>::_nothrow) -> Ret
{
  return _apply_detail::_apply_r_elems<Ret>(
      _apply_tag_elems<Fn, Tag>{FWD(fn)}, FWD(v),
      ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<V>>>{}, FWD(args)...);
}

template <typename Ret, typename Tag, typename Fn, typename V, typename... Args>
  requires(not _some_pack<V>) && (not _some_copack<V>) && (not ::pfn::detail::_tuple_like<V>)
          && ::std::is_invocable_r_v<Ret, Fn, Tag, V, Args...>
[[nodiscard]] constexpr auto _apply_tagged_r(Fn &&fn, V &&v, Args &&...args) //
    noexcept(::std::is_nothrow_invocable_r_v<Ret, Fn, Tag, V, Args...>) -> Ret
{
  return ::pfn::invoke_r<Ret>(FWD(fn), Tag{}, FWD(v), FWD(args)...);
}

// The apply_type arm adapter: the type-indexed dispatch hands (tag, whole value, trailing
// arguments); this re-invokes the user's arm set with the alternative unpacked exactly as
// value-path apply would unpack it, the trailing arguments after the elements.
template <typename Fn> struct _apply_type_fn final {
  Fn &&fn;

  template <typename T, typename V, typename... Args>
  constexpr auto operator()(::std::in_place_type_t<T>, V &&v, Args &&...args) && //
      noexcept(noexcept(::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)},
                                                                FWD(args)...)))
          -> DEDUCED_RETURN(::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)},
                                                                    FWD(args)...))
    requires _some_pack<T> && requires {
      ::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)}, FWD(args)...);
    }
  {
    return ::std::remove_cvref_t<V>::_impl::_apply(FWD(v), _apply_type_elems<Fn, T>{FWD(fn)}, FWD(args)...);
  }

  template <typename T, typename V, typename... Args>
  constexpr auto operator()(::std::in_place_type_t<T>, V &&v, Args &&...args) && //
      noexcept(_apply_detail::_elems_traits<_apply_type_elems<Fn, T>, V, Args...>::_nothrow) ->
      typename _apply_detail::_elems_traits<_apply_type_elems<Fn, T>, V, Args...>::_result::type
    requires(not _some_pack<T>) && ::pfn::detail::_tuple_like<T>
            && (_apply_detail::_elems_traits<_apply_type_elems<Fn, T>, V, Args...>::_invocable)
  {
    return _apply_detail::_apply_elems(_apply_type_elems<Fn, T>{FWD(fn)}, FWD(v),
                                       ::std::make_index_sequence<::std::tuple_size_v<::std::remove_reference_t<V>>>{},
                                       FWD(args)...);
  }

  // The tag is passed as a prvalue, the exact shape the traits above ask about (a named parameter
  // would be an lvalue, splitting the probe from the deed).
  template <typename T, typename V, typename... Args>
  constexpr auto operator()(::std::in_place_type_t<T>, V &&v, Args &&...args) && //
      noexcept(::std::is_nothrow_invocable_v<Fn, ::std::in_place_type_t<T>, V, Args...>)
          -> DEDUCED_RETURN(::std::invoke(FWD(fn), ::std::in_place_type_t<T>{}, FWD(v), FWD(args)...))
    requires(not _some_pack<T>) && (not ::pfn::detail::_tuple_like<T>)
            && ::std::is_invocable_v<Fn, ::std::in_place_type_t<T>, V, Args...>
  {
    return ::std::invoke(FWD(fn), ::std::in_place_type_t<T>{}, FWD(v), FWD(args)...);
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

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_DETAIL_FUNCTIONAL
