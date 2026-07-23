// Copyright (c) 2026 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_DETAIL_MONADIC
#define INCLUDE_FN_DETAIL_MONADIC

#include <fn/detail/fwd.hpp>
#include <libfn_version.hpp>

#include <concepts>

namespace fn::inline LIBFN_VERSION::detail {

template <typename T>
concept _some_monadic_type = _some_expected<T> || _some_optional<T> || _some_choice<T> || _some_just<T>;

template <typename Functor, typename V, typename... Args>
concept _monadic_invocable = _some_monadic_type<V> && ::std::invocable<typename Functor::apply, V, Args...>;

} // namespace fn::inline LIBFN_VERSION::detail

#endif // INCLUDE_FN_DETAIL_MONADIC
