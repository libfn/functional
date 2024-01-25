// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_DETAIL_CLOSURE
#define INCLUDE_FUNCTIONAL_DETAIL_CLOSURE

#include "fwd_macro.hpp"
#include "traits.hpp"

#include <type_traits>
#include <utility>

namespace fn::detail {

template <std::size_t I, typename T> struct _element {
  static_assert(not std::is_rvalue_reference_v<T>);
  T v;
};

template <typename, typename... Ts> struct closure_base;

template <std::size_t... Is, typename... Ts>
struct closure_base<std::index_sequence<Is...>, Ts...> : _element<Is, Ts>... {
  template <typename Self, typename Fn, typename... Args>
  static constexpr auto invoke(Self &&self, Fn &&fn, Args &&...args) noexcept
      -> decltype(FWD(fn)(FWD(args)..., apply_const_t<Self, Ts &&>(
                                            self._element<Is, Ts>::v)...))
  {
    return FWD(fn)(FWD(args)...,
                   apply_const_t<Self, Ts &&>(self._element<Is, Ts>::v)...);
  }
};

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_CLOSURE
