// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_CONCEPTS
#define INCLUDE_FN_CONCEPTS

#include <fn/choice.hpp>
#include <fn/expected.hpp>
#include <fn/monadic.hpp>
#include <fn/optional.hpp>
#include <fn/sum.hpp>

#include <concepts>
#include <type_traits>

namespace fn {

namespace detail {
// A verb that returns a fresh monad BY VALUE relocates what the source carries into it, in the value
// category the source is piped in - an lvalue is copied, an rvalue moved. So the question is not
// whether the carried type is movable: `is_move_constructible_v` would accept a move-only value
// piped as an lvalue, which the body then cannot copy. It is whether the result can be built from
// what the body actually reaches for, which is what these ask - and what the verbs' noexcept specs
// weigh, one asking "can it", the other "can it throw".
template <typename V>
concept _relocatable_value // `type{::std::in_place, FWD(v).value()}`
    = ::std::is_constructible_v<::std::remove_cvref_t<V>, ::std::in_place_t, decltype(::std::declval<V>().value())>;

template <typename V>
concept _relocatable_error // `type{::fn::unexpect, FWD(v).error()}`
    = ::std::is_constructible_v<::std::remove_cvref_t<V>, ::fn::unexpect_t, decltype(::std::declval<V>().error())>;

template <typename V>
concept _relocatable // `return FWD(v);` - the whole monad, hence both sides
    = ::std::is_constructible_v<::std::remove_cvref_t<V>, V>;
} // namespace detail

/**
 * @brief TODO
 * @note `same_kind` is a fundamental concept in category theory; it allows
 * transformation of a value_type, but not an error_type (where applicable)
 *
 * @tparam T TODO
 * @tparam U TODO
 */
template <typename T, typename U>
concept same_kind
    = (some_expected<T> && some_expected<U>
       && ::std::same_as<typename ::std::remove_cvref_t<T>::error_type, typename ::std::remove_cvref_t<U>::error_type>)
      || (some_expected<T> && some_sum<typename ::std::remove_cvref_t<T>::error_type> //
          && some_expected<U> && some_sum<typename ::std::remove_cvref_t<U>::error_type>)
      || (some_optional<T> && some_optional<U>) //
      || (some_choice<T> && some_choice<U>);

/**
 * @brief TODO
 * @note symmetrical to the above
 *
 * @tparam T TODO
 * @tparam U TODO
 */
template <typename T, typename U>
concept same_value_kind
    = (some_expected<T> && some_expected<U>
       && ::std::same_as<typename ::std::remove_cvref_t<T>::value_type, typename ::std::remove_cvref_t<U>::value_type>)
      || (some_expected<T> && some_sum<typename ::std::remove_cvref_t<T>::value_type> //
          && some_expected<U> && some_sum<typename ::std::remove_cvref_t<U>::value_type>)
      || (some_optional<T> && some_optional<U>
          && ::std::same_as<typename ::std::remove_cvref_t<U>::value_type,
                            typename ::std::remove_cvref_t<T>::value_type>)           //
      || (some_optional<T> && some_sum<typename ::std::remove_cvref_t<T>::value_type> //
          && some_optional<U> && some_sum<typename ::std::remove_cvref_t<U>::value_type>)
      || (some_choice<T> && some_choice<U>);

/**
 * @brief TODO
 *
 * @tparam T TODO
 * @tparam U TODO
 */
template <typename T, typename U>
concept same_monadic_type_as = same_kind<T, U> && same_value_kind<T, U>;

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
// The void conjunct is load-bearing: `unexpected<void>` is ill-formed by a class-body mandate,
// which fires during instantiation - outside any immediate context - so without it the question
// hard-errors instead of answering false.
template <class T>
concept convertible_to_unexpected = (not ::std::same_as<T, void>) && requires {
  static_cast<::fn::unexpected<::std::remove_cvref_t<T>>>(::std::declval<T>());
};

/**
 * @brief TODO
 *
 * @tparam T TODO
 * @tparam E TODO
 */
template <class T, typename E>
concept convertible_to_expected = (not ::std::same_as<T, void> && requires {
                                    static_cast<expected<::std::remove_cvref_t<T>, E>>(::std::declval<T>());
                                  }) || (::std::same_as<T, void>);

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
// The same load-bearing void conjunct as `convertible_to_unexpected`'s: `optional<void>` and
// `choice<void>` (below) are likewise ill-formed to instantiate, by a class-body mandate.
template <class T>
concept convertible_to_optional = (not ::std::same_as<T, void>)
                                  && requires { static_cast<optional<::std::remove_cvref_t<T>>>(::std::declval<T>()); };

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <class T>
concept convertible_to_choice
    = (not ::std::same_as<T, void>) && requires { static_cast<choice<::std::remove_cvref_t<T>>>(::std::declval<T>()); };

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <class T>
concept convertible_to_bool = requires { static_cast<bool>(::std::declval<T>()); };

} // namespace fn

#endif // INCLUDE_FN_CONCEPTS
