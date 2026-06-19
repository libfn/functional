// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_MONADIC
#define INCLUDE_FN_DETAIL_MONADIC

#include <fn/detail/fwd.hpp>

#include <concepts>

namespace fn::detail {

template <typename T>
concept _some_monadic_type = _some_expected<T> || _some_optional<T> || _some_choice<T>;

template <typename Functor, typename V, typename... Args>
concept _monadic_invocable = _some_monadic_type<V> && ::std::invocable<typename Functor::apply, V, Args...>;

} // namespace fn::detail

#endif // INCLUDE_FUNCTIONAL_DETAIL_MONADIC
