// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_PACK_IMPL
#define INCLUDE_FUNCTIONAL_DETAIL_PACK_IMPL

#include "functional/detail/functional.hpp"
#include "functional/detail/fwd.hpp"
#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/traits.hpp"

#include <type_traits>
#include <utility>

namespace fn::detail {
template <std::size_t I, typename T> struct _element {
  static_assert(not std::is_rvalue_reference_v<T>);
  T v;
};

template <typename, typename... Ts> struct pack_impl;

template <typename... Ts> struct _pack_append;
template <typename T, typename... Ts>
  requires _some_sum<T>
struct _pack_append<T, Ts...>;
template <typename T, typename... Ts>
  requires(not _some_pack<T>)
struct _pack_append<T, Ts...> {
  using impl = pack_impl<std::index_sequence_for<Ts..., T>, Ts..., T>;
  using type = ::fn::pack<Ts..., T>;
};

template <typename T, typename... Ts> struct _pack_append_pack;
template <typename... Tx, typename... Ts> struct _pack_append_pack<::fn::pack<Tx...>, Ts...> {
  using impl = pack_impl<std::index_sequence_for<Ts..., Tx...>, Ts..., Tx...>;
  using type = ::fn::pack<Ts..., Tx...>;
};
template <typename T, typename... Ts>
  requires _some_pack<T>
struct _pack_append<T, Ts...> {
  using impl = _pack_append_pack<std::remove_cvref_t<T>, Ts...>::impl;
  using type = _pack_append_pack<std::remove_cvref_t<T>, Ts...>::type;
};

template <std::size_t... Is, typename... Ts> struct pack_impl<std::index_sequence<Is...>, Ts...> : _element<Is, Ts>... {
  static constexpr std::size_t size = sizeof...(Is);

  template <typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_sum<Args>)))
  static constexpr auto _swap_invoke(Self &&self, Fn &&fn, Args &&...args) noexcept
      -> ::std::invoke_result<decltype(fn), decltype(args)..., apply_const_lvalue_t<Self, Ts &&>...>::type
    requires(::std::is_invocable<decltype(fn), decltype(args)..., apply_const_lvalue_t<Self, Ts &&>...>::value)
  {
    return ::std::invoke(FWD(fn), FWD(args)...,
                         static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...);
  }

  template <typename Self, typename Fn, typename... Args>
    requires(not(... || (_some_pack<Args> || _some_sum<Args>)))
  static constexpr auto _invoke(Self &&self, Fn &&fn, Args &&...args) noexcept
      -> _invoke_result<decltype(fn), apply_const_lvalue_t<Self, Ts &&>..., decltype(args)...>::type
    requires(_is_invocable<decltype(fn), apply_const_lvalue_t<Self, Ts &&>..., decltype(args)...>::value)
  {
    return ::fn::detail::_invoke(
        FWD(fn), static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(args)...);
  }

  template <typename T> using append_type = _pack_append<T, Ts...>::type;

  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&...args) noexcept //
      -> pack_impl<std::index_sequence<Is..., size>, Ts..., T>
    requires(not _some_sum<T>) && (not _some_pack<T>) && std::is_constructible_v<T, decltype(args)...>
  {
    return {static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., T{FWD(args)...}};
  }

  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&other) noexcept -> //
      typename _pack_append<std::remove_cvref_t<T>, Ts...>::impl
    requires _some_pack<T> && (std::is_same_v<std::remove_cvref_t<decltype(other)>, std::remove_cvref_t<T>>)
  {
    using type = _pack_append<std::remove_cvref_t<T>, Ts...>::impl;
    return FWD(other)._invoke(FWD(other), [&self](auto &&...args) {
      return type{static_cast<apply_const_lvalue_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., FWD(args)...};
    });
  }

  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&...args) noexcept
    requires _some_sum<T>
  = delete;
};

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_PACK_IMPL
