// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FUNCTIONAL_CONCEPTS
#define INCLUDE_FUNCTIONAL_CONCEPTS

#include "functional/choice.hpp"
#include "functional/expected.hpp"
#include "functional/optional.hpp"
#include "functional/sum.hpp"

#include <concepts>
#include <functional>
#include <type_traits>

namespace fn {

template <typename T>
concept some_monadic_type = some_expected<T> || some_optional<T> || some_choice<T>;

// NOTE `same_kind` is a fundamental concept in category theory; it allows
// transformation of a value_type, but not an error_type (where applicable)
template <typename T, typename U>
concept same_kind
    = (some_expected<T> && some_expected<U>
       && std::same_as<typename std::remove_cvref_t<T>::error_type, typename std::remove_cvref_t<U>::error_type>)
      || (some_expected<T> && some_sum<typename std::remove_cvref_t<T>::error_type> //
          && some_expected<U> && some_sum<typename std::remove_cvref_t<U>::error_type>)
      || (some_optional<T> && some_optional<U>) //
      || (some_choice<T> && some_choice<U>);

// NOTE symmetrical to the above
template <typename T, typename U>
concept same_value_kind
    = (some_expected<T> && some_expected<U>
       && std::same_as<typename std::remove_cvref_t<T>::value_type, typename std::remove_cvref_t<U>::value_type>)
      || (some_expected<T> && some_sum<typename std::remove_cvref_t<T>::value_type> //
          && some_expected<U> && some_sum<typename std::remove_cvref_t<U>::value_type>)
      || (some_optional<T> && some_optional<U>
          && std::same_as<typename std::remove_cvref_t<U>::value_type, typename std::remove_cvref_t<T>::value_type>) //
      || (some_choice<T> && some_choice<U>);

template <typename T, typename U>
concept same_monadic_type_as = same_kind<T, U> && same_value_kind<T, U>;

template <class T>
concept convertible_to_unexpected
    = requires { static_cast<std::unexpected<std::remove_cvref_t<T>>>(std::declval<T>()); };

template <class T, typename E>
concept convertible_to_expected
    = (not std::same_as<T, void> && requires { static_cast<expected<std::remove_cvref_t<T>, E>>(std::declval<T>()); })
      || (std::same_as<T, void>);

template <class T>
concept convertible_to_optional = requires { static_cast<optional<std::remove_cvref_t<T>>>(std::declval<T>()); };

template <class T>
concept convertible_to_choice = requires { static_cast<choice<std::remove_cvref_t<T>>>(std::declval<T>()); };

template <class T>
concept convertible_to_bool = requires { static_cast<bool>(std::declval<T>()); };

} // namespace fn

#endif // INCLUDE_FUNCTIONAL_CONCEPTS
