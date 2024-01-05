// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_CONCEPTS
#define INCLUDE_FUNCTIONAL_CONCEPTS

#include "functional/detail/concepts.hpp"

namespace fn {
template <typename T>
concept some_expected = detail::is_some_expected<T>;

template <typename T>
concept some_optional = detail::is_some_optional<T>;

template <typename T>
concept some_monadic_type = some_expected<T> || some_optional<T>;

template <typename T>
concept some_tuple = detail::is_some_tuple<T>;
} // namespace fn

#endif // INCLUDE_FUNCTIONAL_CONCEPTS
