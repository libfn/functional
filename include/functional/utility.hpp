// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_UTILITY
#define INCLUDE_FUNCTIONAL_UTILITY

#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/traits.hpp"

namespace fn {
template <typename T> using as_value_t = decltype(detail::_as_value<T>);

// NOTE: Unlike apply_const_lvalue_t above, this is not exact: prvalue parameters are
// returned as xvalue. This is meant to disable copying of the return value.
template <typename T> [[nodiscard]] constexpr auto apply_const_lvalue(auto &&v) noexcept -> decltype(auto)
{
  return static_cast<apply_const_lvalue_t<T, decltype(v)>>(v);
}

template <typename... Ts> struct overload final : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts const &...) -> overload<Ts...>;

// Preferred make lift function using {}
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make(Args &&...args) -> T
  requires requires(Args &&...args) { T{FWD(args)...}; }
{
  return T{FWD(args)...};
}

// Fallback to () construction if {} is not available
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make(Args &&...args) -> T
  requires requires(Args &&...args) { T(FWD(args)...); } && (not requires(Args &&...args) { T{FWD(args)...}; })
{
  return T(FWD(args)...);
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_UTILITY
