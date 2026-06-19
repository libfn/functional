// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_MONADIC
#define INCLUDE_FN_MONADIC

#include <fn/detail/monadic.hpp>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_monadic_type = detail::_some_monadic_type<T>;

/**
 * @brief TODO
 *
 * @tparam Functor TODO
 * @tparam V TODO
 * @tparam Args TODO
 */
template <typename Functor, typename V, typename... Args>
concept monadic_invocable = detail::_monadic_invocable<Functor, V, Args...>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_MONADIC
