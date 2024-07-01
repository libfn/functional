// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_UTILITY
#define INCLUDE_FUNCTIONAL_UTILITY

#include "functional/detail/fwd_macro.hpp"
#include "functional/detail/traits.hpp"

namespace fn {
/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T> using as_value_t = decltype(detail::_as_value<T>);

/**
 * @brief TODO
 *
 * @note Unlike apply_const_lvalue_t above, this is not exact: prvalue parameters are
 * returned as xvalue. This is meant to disable copying of the return value.
 *
 * @tparam T TODO
 * @param v TODO
 * @return TODO
 */
template <typename T> [[nodiscard]] constexpr auto apply_const_lvalue(auto &&v) noexcept -> decltype(auto)
{
  return static_cast<apply_const_lvalue_t<T, decltype(v)>>(v);
}

template <typename... Ts> struct overload final : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts const &...) -> overload<Ts...>;

/**
 * @brief Preferred make lift function using {}
 *
 * @tparam T TODO
 * @tparam Args TODO
 * @param args TODO
 */
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make(Args &&...args) -> T
  requires requires(Args &&...args) { T{FWD(args)...}; }
{
  return T{FWD(args)...};
}

/**
 * @brief Fallback to () construction if {} is not available
 *
 * @tparam T TODO
 * @tparam Args TODO
 * @param args TODO
 */
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make(Args &&...args) -> T
  requires requires(Args &&...args) { T(FWD(args)...); } && (not requires(Args &&...args) { T{FWD(args)...}; })
{
  return T(FWD(args)...);
}

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_UTILITY
