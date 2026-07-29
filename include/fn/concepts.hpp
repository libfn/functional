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
#include <libfn_version.hpp>

#include <concepts>
#include <type_traits>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {

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
 * @brief Checks if two carriers are of the same monadic family, with compatible error sides
 *
 * What `and_then` and the other success-path operations hold a callback's result to: the value
 * side may change freely, the family may not. For `expected` the error sides must also agree -
 * identical plain types, any two graded (copack) sides, or a plain side meeting its own singular
 * lift `copack<E>`, either way round. Any two `optional`s, any two `choice`s and any two `just`s
 * are the same kind.
 *
 * @tparam T Carrier type, possibly cv-ref qualified
 * @tparam U Carrier type, possibly cv-ref qualified
 */
template <typename T, typename U>
concept same_kind
    = (some_expected<T> && some_expected<U>
       && ::std::same_as<typename ::std::remove_cvref_t<T>::error_type, typename ::std::remove_cvref_t<U>::error_type>)
      || (some_expected<T> && some_copack<typename ::std::remove_cvref_t<T>::error_type> //
          && some_expected<U> && some_copack<typename ::std::remove_cvref_t<U>::error_type>)
      || (some_expected<T>
          && (not some_copack<typename ::std::remove_cvref_t<T>::error_type>) // the singular lift
          &&some_expected<U>
          && ::std::is_same_v<typename ::std::remove_cvref_t<U>::error_type,
                              copack<typename ::std::remove_cvref_t<T>::error_type>>)
      || (some_expected<U>
          && (not some_copack<typename ::std::remove_cvref_t<U>::error_type>) // ... and its mirror
          &&some_expected<T>
          && ::std::is_same_v<typename ::std::remove_cvref_t<T>::error_type,
                              copack<typename ::std::remove_cvref_t<U>::error_type>>)
      || (some_optional<T> && some_optional<U>) //
      || (some_choice<T> && some_choice<U>)     //
      || (some_just<T> && some_just<U>);

/**
 * @brief The mirror of `same_kind`: the value side is pinned and the error side may vary
 *
 * What the error-side operations hold a callback's result to. Value sides agree when identical,
 * when both are graded (copack), or - on `expected` - when a plain non-void side meets its own
 * singular lift `copack<T>`, either way round. Any two `choice`s qualify; never two carriers of
 * different families.
 *
 * @tparam T Carrier type, possibly cv-ref qualified
 * @tparam U Carrier type, possibly cv-ref qualified
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
      || (some_expected<T>
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<T>::value_type>) // the singular lift
          &&(not some_copack<typename ::std::remove_cvref_t<T>::value_type>)
          && some_expected<U>
          && ::std::is_same_v<typename ::std::remove_cvref_t<U>::value_type,
                              copack<typename ::std::remove_cvref_t<T>::value_type>>)
      || (some_expected<U>
          && (not ::std::is_void_v<typename ::std::remove_cvref_t<U>::value_type>) // ... and its mirror
          &&(not some_copack<typename ::std::remove_cvref_t<U>::value_type>)
          && some_expected<T>
          && ::std::is_same_v<typename ::std::remove_cvref_t<T>::value_type,
                              copack<typename ::std::remove_cvref_t<U>::value_type>>)
      || (some_choice<T> && some_choice<U>);

/**
 * @brief Checks if two carriers agree on family, value side and error side alike
 *
 * The conjunction of `same_kind` and `same_value_kind`.
 *
 * @tparam T Carrier type, possibly cv-ref qualified
 * @tparam U Carrier type, possibly cv-ref qualified
 */
template <typename T, typename U>
concept same_monadic_type_as = same_kind<T, U> && same_value_kind<T, U>;

/**
 * @brief Checks if `fn::unexpected` over the decayed type can be built from the value by a cast
 *
 * A `void` answers false - `unexpected<void>` is ill-formed to instantiate, and the concept
 * answers instead. An immovable value answers false as well: there is nothing to move or copy
 * into the carrier.
 *
 * @tparam T Type of the value, possibly cv-ref qualified
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
 * @brief Checks if an `expected` over the decayed type can be built from the value by a cast
 *
 * Unlike the other `convertible_to_*` concepts here, `void` is admitted outright by its own arm:
 * `expected<void, E>` is a legitimate carrier with no value to convert.
 *
 * @tparam T Type of the value, possibly cv-ref qualified
 * @tparam E Error type of the target `expected`
 */
template <class T, typename E>
concept convertible_to_expected
    = (not ::std::is_void_v<T> && requires { static_cast<expected<::std::remove_cvref_t<T>, E>>(::std::declval<T>()); })
      || (::std::is_void_v<T>);

/**
 * @brief Checks if an `optional` over the decayed type can be built from the value by a cast
 *
 * A `void` answers false - `optional<void>` is ill-formed to instantiate, and the concept answers
 * instead.
 *
 * @tparam T Type of the value, possibly cv-ref qualified
 */
// The same load-bearing void conjunct as `convertible_to_unexpected`'s: `optional<void>` and
// `choice<void>` (below) are likewise ill-formed to instantiate, by a class-body mandate.
template <class T>
concept convertible_to_optional
    = (not ::std::is_void_v<T>) && requires { static_cast<optional<::std::remove_cvref_t<T>>>(::std::declval<T>()); };

/**
 * @brief Checks if a `choice` over the decayed type can be built from the value by a cast
 *
 * A `void` answers false - `choice<void>` is ill-formed to instantiate, and the concept answers
 * instead.
 *
 * @tparam T Type of the value, possibly cv-ref qualified
 */
template <class T>
concept convertible_to_choice
    = (not ::std::is_void_v<T>) && requires { static_cast<choice<::std::remove_cvref_t<T>>>(::std::declval<T>()); };

/**
 * @brief Checks if a carrier's error side is uninhabited - it can never hold an error
 *
 * The identity `expected`, and only it: `choice` and `just` have no error channel to empty, so they
 * answer false here while `some_identity` below accepts all three.
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_empty_error = some_expected<T> && empty_copack<typename ::std::remove_cvref_t<T>::error_type>;

/**
 * @brief Checks if a carrier's value side is uninhabited - it can never hold a value
 *
 * An `optional<copack<>>`, which is never engaged, or an `expected<copack<>, E>`, which always holds
 * its error: every value-side operation over one is the identity, and its callback is neither
 * invoked nor instantiated. `optional` belongs here where it cannot belong to the mirror above - its
 * empty state is a state that is, not a channel that can never engage.
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <typename T>
concept some_empty_value
    = (some_expected<T> || some_optional<T>) && empty_copack<typename ::std::remove_cvref_t<T>::value_type>;

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
concept some_identity = some_choice<T> || some_just<T> || some_empty_error<T>;

/**
 * @brief Checks if the type casts to `bool` - what `filter` holds its predicate's result to
 *
 * An explicit `operator bool` suffices: the concept casts, it does not ask for an implicit
 * conversion.
 *
 * @tparam T Type to check, possibly cv-ref qualified
 */
template <class T>
concept convertible_to_bool = requires { static_cast<bool>(::std::declval<T>()); };

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_CONCEPTS
