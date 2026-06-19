// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_MONADIC
#define INCLUDE_FN_MONADIC

#include <fn/fwd.hpp>

#include <concepts>

namespace fn {

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <typename T>
concept some_monadic_type = detail::_some_expected<T> || detail::_some_optional<T> || detail::_some_choice<T>;

/**
 * @brief TODO
 *
 * @tparam Functor TODO
 * @tparam V TODO
 * @tparam Args TODO
 */
template <typename Functor, typename V, typename... Args>
concept monadic_invocable = some_monadic_type<V> && ::std::invocable<typename Functor::apply, V, Args...>;

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_MONADIC
