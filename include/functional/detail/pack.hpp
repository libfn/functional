// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_PACK
#define INCLUDE_FUNCTIONAL_DETAIL_PACK

#include "fwd_macro.hpp"
#include "traits.hpp"

#include <functional>
#include <type_traits>
#include <utility>

namespace fn::detail {

template <std::size_t I, typename T> struct _element {
  static_assert(not std::is_rvalue_reference_v<T>);
  T v;
};

template <typename, typename... Ts> struct pack_base;

template <std::size_t... Is, typename... Ts> struct pack_base<std::index_sequence<Is...>, Ts...> : _element<Is, Ts>... {
  static constexpr std::size_t size() noexcept { return (0 + ... + ((Is ^ Is) + 1)); }

  template <typename Self, typename Fn, typename... Args>
  static constexpr auto _invoke(Self &&self, Fn &&fn, Args &&...args) noexcept
      -> std::invoke_result_t<decltype(fn), decltype(args)..., apply_const_t<Self, Ts &&>...>
  {
    return std::invoke(FWD(fn), FWD(args)...,
                       static_cast<apply_const_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)...);
  }

  template <typename T, typename Self>
  static constexpr auto _append(Self &&self, auto &&...args) noexcept
      -> pack_base<std::index_sequence<Is..., size()>, Ts..., T>
    requires std::is_constructible_v<T, decltype(args)...>
  {
    return {static_cast<apply_const_t<Self, Ts &&>>(FWD(self)._element<Is, Ts>::v)..., T{FWD(args)...}};
  }
};

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_PACK
