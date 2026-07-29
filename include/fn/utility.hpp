// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_UTILITY
#define INCLUDE_FN_UTILITY

#include <fn/detail/traits.hpp>
#include <libfn_version.hpp>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief The type an argument of type `T` is stored as: a non-empty lvalue stays an lvalue
 *        reference, everything else becomes a value
 *
 * What `fn::functor` stores a pipeline operation's arguments as: an rvalue is kept alive by value,
 * while a stateless (empty) callable is stored by value even when passed as an lvalue - it has no
 * state whose identity would matter. A non-empty lvalue stays a reference, which is what lets an
 * immovable callable participate in a pipeline.
 *
 * @tparam T Type of the argument, cv-ref qualified as deduced
 */
template <typename T> using as_value_t = decltype(detail::_as_value<T>);

/**
 * @brief Forwards `v` with the constness and value category of `T` applied
 *
 * @note Unlike `apply_const_lvalue_t`, this is not exact: prvalue parameters are
 * returned as xvalue. This is meant to disable copying of the return value.
 *
 * @tparam T Type whose constness and value category are applied
 * @param v Value to forward
 * @return `v`, cast to `apply_const_lvalue_t<T, decltype(v)>`
 */
template <typename T> [[nodiscard]] constexpr auto apply_const_lvalue(auto &&v) noexcept -> decltype(auto)
{
  return static_cast<apply_const_lvalue_t<T, decltype(v)>>(v);
}

/**
 * @brief Fuses callables into a single overload set
 *
 * The routing helper for multidispatch: `apply` over a `copack` or `choice` selects the arm by
 * ordinary overload resolution, and a missing arm is a compile error. An unconstrained `[](auto)`
 * arm serves every alternative, which defeats that exhaustiveness check; where one arm should
 * serve several alternatives, constrain it with a concept instead.
 *
 * @tparam Ts Callables whose `operator()` overloads combine
 */
template <typename... Ts> struct overload final : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> overload(Ts const &...) -> overload<Ts...>;

/**
 * @brief Preferred make lift function using {}
 *
 * @tparam T Type to construct
 * @param args Arguments to construct the `T` from
 * @return The constructed value
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
 * @tparam T Type to construct
 * @param args Arguments to construct the `T` from
 * @return The constructed value
 */
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make(Args &&...args) -> T
  requires requires(Args &&...args) { T(FWD(args)...); } && (not requires(Args &&...args) { T{FWD(args)...}; })
{
  return T(FWD(args)...);
}

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_UTILITY
