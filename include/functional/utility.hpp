// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_UTILITY
#define INCLUDE_FUNCTIONAL_UTILITY

#include "functional/detail/closure.hpp"
#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/traits.hpp"

namespace fn {
template <typename T> using as_value_t = decltype(detail::_as_value<T>);

template <typename T, typename V> using apply_const_t = detail::apply_const_t<T, V>;

// NOTE: Unlike apply_const_t above, this is not exact: prvalue parameters are
// returned as xvalue. This is meant to disable copying of the return value.
template <typename T> constexpr auto apply_const(auto &&v) noexcept -> decltype(auto)
{
  return static_cast<apply_const_t<T, decltype(v)>>(v);
}

template <typename... Ts> struct closure : detail::closure_base<std::index_sequence_for<Ts...>, Ts...> {};

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_UTILITY
