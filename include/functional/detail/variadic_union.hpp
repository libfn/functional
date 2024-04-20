// Copyright (c) 2024 Gašper Ažman, Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_VARIADIC_UNION
#define INCLUDE_FUNCTIONAL_DETAIL_VARIADIC_UNION

#include "functional/detail/functional.hpp"
#include "functional/detail/fwd_macro.hpp"

#include <type_traits>
#include <utility>

namespace fn::detail {

template <typename T> constexpr bool _is_in_place_type = false;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> &> = true;
template <typename T> constexpr bool _is_in_place_type<::std::in_place_type_t<T> const &> = true;
template <typename T>
concept _some_in_place_type = _is_in_place_type<T &>;

template <typename Fn, typename T> constexpr inline bool _is_tst_invocable = false;
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> &>
    = (... && _is_invocable<Fn, ::std::in_place_type_t<Ts>, Ts &>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> const &>
    = (... && _is_invocable<Fn, ::std::in_place_type_t<Ts>, Ts const &>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> &&>
    = (... && _is_invocable<Fn, ::std::in_place_type_t<Ts>, Ts &&>::value);
template <typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_tst_invocable<Fn, Tpl<Ts...> const &&>
    = (... && _is_invocable<Fn, ::std::in_place_type_t<Ts>, Ts const &&>::value);
template <typename Fn, typename T>
concept _typelist_type_invocable = _is_tst_invocable<Fn, T &&>;

template <typename R, typename Fn, typename T> constexpr inline bool _is_rtst_invocable = false;
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> &>
    = (... && _is_invocable_r<R, Fn, ::std::in_place_type_t<Ts>, Ts &>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> const &>
    = (... && _is_invocable_r<R, Fn, ::std::in_place_type_t<Ts>, Ts const &>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> &&>
    = (... && _is_invocable_r<R, Fn, ::std::in_place_type_t<Ts>, Ts &&>::value);
template <typename R, typename Fn, template <typename...> typename Tpl, typename... Ts>
constexpr inline bool _is_rtst_invocable<R, Fn, Tpl<Ts...> const &&>
    = (... && _is_invocable_r<R, Fn, ::std::in_place_type_t<Ts>, Ts const &&>::value);
template <typename R, typename Fn, typename T>
concept _typelist_type_invocable_r = _is_rtst_invocable<R, Fn, T &&>;

template <typename... Ts> union variadic_union;
template <> union variadic_union<>; // Intentionally incomplete

template <typename... Ts> constexpr bool _is_variadic_union = false;
template <typename... Ts> constexpr bool _is_variadic_union<variadic_union<Ts...> &> = true;
template <typename... Ts> constexpr bool _is_variadic_union<variadic_union<Ts...> const &> = true;

template <typename T>
concept some_variadic_union = _is_variadic_union<T &>;

template <typename T0> union variadic_union<T0> final {
  static_assert(not _some_in_place_type<T0>);
  using t0 = T0;
  T0 v0;

  template <typename T>
  static constexpr bool has_type //
      = std::is_same_v<T, T0>;
  static constexpr std::size_t size = 1;

  constexpr ~variadic_union() {}
};

template <typename T0, typename T1> union variadic_union<T0, T1> final {
  static_assert(not _some_in_place_type<T0>);
  static_assert(not _some_in_place_type<T1>);
  static_assert(not std::is_same_v<T0, T1>);

  using t0 = T0;
  using t1 = T1;
  T0 v0;
  T1 v1;

  template <typename T>
  static constexpr bool has_type //
      = std::is_same_v<T, T0> || std::is_same_v<T, T1>;
  static constexpr std::size_t size = 2;

  constexpr ~variadic_union() {}
};

template <typename T0, typename T1, typename T2> union variadic_union<T0, T1, T2> final {
  static_assert(not _some_in_place_type<T0>);
  static_assert(not _some_in_place_type<T1>);
  static_assert(not _some_in_place_type<T2>);
  static_assert(not std::is_same_v<T0, T1>);
  static_assert(not std::is_same_v<T0, T2>);
  static_assert(not std::is_same_v<T1, T2>);

  using t0 = T0;
  using t1 = T1;
  using t2 = T2;
  T0 v0;
  T1 v1;
  T2 v2;

  template <typename T>
  static constexpr bool has_type //
      = std::is_same_v<T, T0> || std::is_same_v<T, T1> || std::is_same_v<T, T2>;
  static constexpr std::size_t size = 3;

  constexpr ~variadic_union() {}
};

template <typename T0, typename T1, typename T2, typename T3> union variadic_union<T0, T1, T2, T3> final {
  static_assert(not _some_in_place_type<T0>);
  static_assert(not _some_in_place_type<T1>);
  static_assert(not _some_in_place_type<T2>);
  static_assert(not _some_in_place_type<T3>);
  static_assert(not std::is_same_v<T0, T1>);
  static_assert(not std::is_same_v<T0, T2>);
  static_assert(not std::is_same_v<T1, T2>);
  static_assert(not std::is_same_v<T0, T3>);
  static_assert(not std::is_same_v<T1, T3>);
  static_assert(not std::is_same_v<T2, T3>);

  using t0 = T0;
  using t1 = T1;
  using t2 = T2;
  using t3 = T3;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;

  template <typename T>
  static constexpr bool has_type //
      = std::is_same_v<T, T0> || std::is_same_v<T, T1> || std::is_same_v<T, T2> || std::is_same_v<T, T3>;
  static constexpr std::size_t size = 4;

  constexpr ~variadic_union() {}
};

template <typename T0, typename T1, typename T2, typename T3, typename... Ts>
  requires(sizeof...(Ts) > 0)
union variadic_union<T0, T1, T2, T3, Ts...> final {
  static_assert(not _some_in_place_type<T0>);
  static_assert(not _some_in_place_type<T1>);
  static_assert(not _some_in_place_type<T2>);
  static_assert(not _some_in_place_type<T3>);
  static_assert((... && (not _some_in_place_type<Ts>)));
  static_assert(not std::is_same_v<T0, T1>);
  static_assert(not std::is_same_v<T0, T2>);
  static_assert(not std::is_same_v<T1, T2>);
  static_assert(not std::is_same_v<T0, T3>);
  static_assert(not std::is_same_v<T1, T3>);
  static_assert(not std::is_same_v<T2, T3>);
  static_assert(not(... || std::is_same_v<T0, Ts>));
  static_assert(not(... || std::is_same_v<T1, Ts>));
  static_assert(not(... || std::is_same_v<T2, Ts>));
  static_assert(not(... || std::is_same_v<T3, Ts>));

  using t0 = T0;
  using t1 = T1;
  using t2 = T2;
  using t3 = T3;
  using more_t = variadic_union<Ts...>;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;
  more_t more;

  template <typename T>
  static constexpr bool has_type //
      = std::is_same_v<T, T0> || std::is_same_v<T, T1> || std::is_same_v<T, T2> || std::is_same_v<T, T3>
        || variadic_union<Ts...>::template has_type<T>;
  static constexpr std::size_t size = 4 + more_t::size;

  constexpr ~variadic_union() {}
};

template <typename T, typename U>
[[nodiscard]] constexpr auto *ptr_variadic_union(some_variadic_union auto &&v) noexcept
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U>
           && (U::template has_type<T>) && std::is_same_v<T, typename U::t0>
{
  return &v.v0;
}

template <typename T, typename U>
[[nodiscard]] constexpr auto *ptr_variadic_union(some_variadic_union auto &&v) noexcept
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U>
           && (U::template has_type<T>) && std::is_same_v<T, typename U::t1>
{
  return &v.v1;
}

template <typename T, typename U>
[[nodiscard]] constexpr auto *ptr_variadic_union(some_variadic_union auto &&v) noexcept
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U>
           && (U::template has_type<T>) && std::is_same_v<T, typename U::t2>
{
  return &v.v2;
}

template <typename T, typename U>
[[nodiscard]] constexpr auto *ptr_variadic_union(some_variadic_union auto &&v) noexcept
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U>
           && (U::template has_type<T>) && std::is_same_v<T, typename U::t3>
{
  return &v.v3;
}

template <typename T, typename U>
[[nodiscard]] constexpr auto *ptr_variadic_union(some_variadic_union auto &&v) noexcept
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U>
           && (U::template has_type<T>) && (U::more_t::template has_type<T>)
{
  return ptr_variadic_union<T, typename U::more_t>(v.more);
}

template <typename T, typename U>
[[nodiscard]] constexpr auto make_variadic_union(auto &&...args)
  requires(U::template has_type<T>) && std::is_same_v<T, typename U::t0>
{
  return U{.v0 = T{FWD(args)...}};
}

template <typename T, typename U>
[[nodiscard]] constexpr auto make_variadic_union(auto &&...args)
  requires(U::template has_type<T>) && std::is_same_v<T, typename U::t1>
{
  return U{.v1 = T{FWD(args)...}};
}

template <typename T, typename U>
[[nodiscard]] constexpr auto make_variadic_union(auto &&...args)
  requires(U::template has_type<T>) && std::is_same_v<T, typename U::t2>
{
  return U{.v2 = T{FWD(args)...}};
}

template <typename T, typename U>
[[nodiscard]] constexpr auto make_variadic_union(auto &&...args)
  requires(U::template has_type<T>) && std::is_same_v<T, typename U::t3>
{
  return U{.v3 = T{FWD(args)...}};
}

template <typename T, typename U>
[[nodiscard]] constexpr U make_variadic_union(auto &&...args)
  requires(U::template has_type<T>) && (U::more_t::template has_type<T>)
{
  return U{.more = make_variadic_union<T, typename U::more_t>(FWD(args)...)};
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable_r<R, Fn, decltype(v)>        //
           && (U::size == 1) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v0));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t,
                                                   Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable_r<R, Fn, decltype(v)>   //
           && (U::size == 1) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable<Fn, decltype(v)>             //
           && (U::size == 1) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), FWD(v).v0);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable<Fn, decltype(v)>        //
           && (U::size == 1) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable_r<R, Fn, decltype(v)>        //
           && (U::size == 2) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v1));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t,
                                                   Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable_r<R, Fn, decltype(v)>   //
           && (U::size == 2) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable<Fn, decltype(v)>             //
           && (U::size == 2) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), FWD(v).v1);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable<Fn, decltype(v)>        //
           && (U::size == 2) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable_r<R, Fn, decltype(v)>        //
           && (U::size == 3) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v1));
  else if (index == 2)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v2));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t,
                                                   Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable_r<R, Fn, decltype(v)>   //
           && (U::size == 3) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1));
  else if (index == 2)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t2>, FWD(v).v2));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable<Fn, decltype(v)>             //
           && (U::size == 3) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), FWD(v).v1);
  else if (index == 2)
    return (void)_invoke(FWD(fn), FWD(v).v2);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable<Fn, decltype(v)>        //
           && (U::size == 3) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1);
  else if (index == 2)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t2>, FWD(v).v2);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable_r<R, Fn, decltype(v)>        //
           && (U::size == 4) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v1));
  else if (index == 2)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v2));
  else if (index == 3)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v3));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t,
                                                   Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable_r<R, Fn, decltype(v)>   //
           && (U::size == 4) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1));
  else if (index == 2)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t2>, FWD(v).v2));
  else if (index == 3)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t3>, FWD(v).v3));
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable<Fn, decltype(v)>             //
           && (U::size == 4) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), FWD(v).v1);
  else if (index == 2)
    return (void)_invoke(FWD(fn), FWD(v).v2);
  else if (index == 3)
    return (void)_invoke(FWD(fn), FWD(v).v3);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable<Fn, decltype(v)>        //
           && (U::size == 4) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1);
  else if (index == 2)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t2>, FWD(v).v2);
  else if (index == 3)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t3>, FWD(v).v3);
  std::unreachable();
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable_r<R, Fn, decltype(v)>        //
           && (U::size > 4) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v1));
  else if (index == 2)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v2));
  else if (index == 3)
    return static_cast<R>(_invoke(FWD(fn), FWD(v).v3));
  else
    return invoke_variadic_union<R, typename U::more_t>(FWD(v).more, index - 4, FWD(fn));
}

template <typename R, typename U, typename Fn>
[[nodiscard]] constexpr auto invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t,
                                                   Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable_r<R, Fn, decltype(v)>   //
           && (U::size > 4) && (not std::is_same_v<void, R>)
{
  if (index == 0)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0));
  else if (index == 1)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1));
  else if (index == 2)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t2>, FWD(v).v2));
  else if (index == 3)
    return static_cast<R>(_invoke(FWD(fn), std::in_place_type<typename U::t3>, FWD(v).v3));
  else
    return invoke_variadic_union<R, typename U::more_t>(FWD(v).more, index - 4, std::in_place, FWD(fn));
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_invocable<Fn, decltype(v)>             //
           && (U::size > 4) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), FWD(v).v1);
  else if (index == 2)
    return (void)_invoke(FWD(fn), FWD(v).v2);
  else if (index == 3)
    return (void)_invoke(FWD(fn), FWD(v).v3);
  else
    return invoke_variadic_union<R, typename U::more_t>(FWD(v).more, index - 4, FWD(fn));
}

template <typename R, typename U, typename Fn>
constexpr void invoke_variadic_union(some_variadic_union auto &&v, std::size_t index, std::in_place_t, Fn &&fn)
  requires std::is_same_v<std::remove_cvref_t<decltype(v)>, U> //
           && _typelist_type_invocable<Fn, decltype(v)>        //
           && (U::size > 4) && (std::is_same_v<void, R>)
{
  if (index == 0)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t0>, FWD(v).v0);
  else if (index == 1)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t1>, FWD(v).v1);
  else if (index == 2)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t2>, FWD(v).v2);
  else if (index == 3)
    return (void)_invoke(FWD(fn), std::in_place_type<typename U::t3>, FWD(v).v3);
  else
    return invoke_variadic_union<R, typename U::more_t>(FWD(v).more, index - 4, std::in_place, FWD(fn));
}

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_VARIADIC_UNION
