// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_CONCEPTS
#define INCLUDE_FN_CONCEPTS

#include <fn/choice.hpp>
#include <fn/copack.hpp>
#include <fn/expected.hpp>
#include <fn/just.hpp>
#include <fn/monadic.hpp>
#include <fn/optional.hpp>

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
      || (some_expected<T> && some_copack<typename ::std::remove_cvref_t<T>::error_type> //
          && some_expected<U> && some_copack<typename ::std::remove_cvref_t<U>::error_type>)
      || (some_optional<T> && some_optional<U>) //
      || (some_choice<T> && some_choice<U>)     //
      || (some_just<T> && some_just<U>);

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
      || (some_expected<T> && some_copack<typename ::std::remove_cvref_t<T>::value_type> //
          && some_expected<U> && some_copack<typename ::std::remove_cvref_t<U>::value_type>)
      || (some_optional<T> && some_optional<U>
          && ::std::same_as<typename ::std::remove_cvref_t<U>::value_type,
                            typename ::std::remove_cvref_t<T>::value_type>)              //
      || (some_optional<T> && some_copack<typename ::std::remove_cvref_t<T>::value_type> //
          && some_optional<U> && some_copack<typename ::std::remove_cvref_t<U>::value_type>)
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
// hard-errors instead of answering false. `remove_cvref_t` folds cv-qualified void back to plain
// `void`, so the guard must cover every cv-flavour - `is_void_v`, not a `same_as` test.
template <class T>
concept convertible_to_unexpected = (not ::std::is_void_v<T>) && requires {
  static_cast<::fn::unexpected<::std::remove_cvref_t<T>>>(::std::declval<T>());
};

/**
 * @brief TODO
 *
 * @tparam T TODO
 * @tparam E TODO
 */
template <class T, typename E>
concept convertible_to_expected
    = (not ::std::is_void_v<T> && requires { static_cast<expected<::std::remove_cvref_t<T>, E>>(::std::declval<T>()); })
      || (::std::is_void_v<T>);

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
// The same load-bearing void conjunct as `convertible_to_unexpected`'s: `optional<void>` and
// `choice<void>` (below) are likewise ill-formed to instantiate, by a class-body mandate.
template <class T>
concept convertible_to_optional
    = (not ::std::is_void_v<T>) && requires { static_cast<optional<::std::remove_cvref_t<T>>>(::std::declval<T>()); };

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <class T>
concept convertible_to_choice
    = (not ::std::is_void_v<T>) && requires { static_cast<choice<::std::remove_cvref_t<T>>>(::std::declval<T>()); };

/**
 * @brief Checks if a type is an identity carrier - a monad which never short-circuits
 *
 * The equivalence class of the family's unit: `choice` (no error channel at all), `just` (the
 * canonical minimal carrier) and an `expected` whose error is the empty copack (a channel that
 * can never engage). Binding across the cluster is lossless exactly because the channels a
 * carrier switch drops are uninhabited.
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_identity = some_choice<T> || some_just<T>
                        || (some_expected<T> && empty_copack<typename ::std::remove_cvref_t<T>::error_type>);

/**
 * @brief TODO
 *
 * @tparam T TODO
 */
template <class T>
concept convertible_to_bool = requires { static_cast<bool>(::std::declval<T>()); };

} // namespace fn

#endif // INCLUDE_FN_CONCEPTS
