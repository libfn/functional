// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_INSPECT
#define INCLUDE_FN_INSPECT

#include <fn/concepts.hpp>
#include <fn/expected.hpp>
#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <fn/just.hpp>
#include <libfn_version.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief Checks if the monadic type can be used with the `inspect` operation
 *
 * @tparam Fn The function to observe the value
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_inspect //
    = (some_expected_non_void<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), ::std::as_const(v).value()) } -> ::std::same_as<void>;
      }) || (some_expected_void<V> && requires(Fn &&fn) {
        { ::fn::apply(FWD(fn)) } -> ::std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), ::std::as_const(v).value()) } -> ::std::same_as<void>;
      }) || (some_choice<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), ::std::as_const(v).value()) } -> ::std::same_as<void>;
      }) || (some_just<V> && (not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>) && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), ::std::as_const(v).value()) } -> ::std::same_as<void>;
      }) || (some_just<V> && ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type> && requires(Fn &&fn) {
        { ::fn::apply(FWD(fn)) } -> ::std::same_as<void>;
      });

/**
 * @brief Observe the value for side-effects, passing the operand through unchanged
 *
 * The callback receives the value as const and must return `void`: observation can neither mutate
 * the operand nor replace it. Where the carrier can be empty or hold an error, the callback runs
 * only when a value is present; on `choice` and `just` it always runs; over an uninhabited value
 * side it is neither invoked nor instantiated.
 *
 * Use through the `fn::inspect` nielbloid.
 */
constexpr inline struct inspect_t final {
  /**
   * @brief Observe the value for side-effects, passing the operand through unchanged
   * @param fn The function to observe the value; invoked with no arguments where the value is `void`
   * @return A functor that will execute the function on the value
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const noexcept(noexcept(functor<inspect_t, decltype(fn)>{FWD(fn)}))
      -> functor<inspect_t, decltype(fn)>
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect = {}; ///< Observes the value in passing: `x | inspect(f)`

struct inspect_t::apply final {
  /**
   * @brief Observes the value for side-effects, when one is present
   *
   * @param v The monad
   * @param fn The function to observe the value
   * @return The operand, forwarded unchanged
   */
  template <some_expected_non_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(::std::as_const(v).value())>) -> V &&
    requires applicable_inspect<Fn &&, V &&>
  {
    if (v.has_value()) {
      ::fn::apply(FWD(fn), ::std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }

  template <some_expected_void V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(::fn::is_nothrow_applicable_v<Fn>) -> V &&
    requires applicable_inspect<Fn &&, V &&>
  {
    if (v.has_value()) {
      ::fn::apply(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }

  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(::std::as_const(v).value())>) -> V &&
    requires applicable_inspect<Fn &&, V &&>
  {
    if (v.has_value()) {
      ::fn::apply(FWD(fn), ::std::as_const(v).value()); // side-effects only
    }
    return FWD(v);
  }

  template <some_choice V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(::std::as_const(v).value())>) -> V &&
    requires applicable_inspect<Fn &&, V &&>
  {
    ::fn::apply(FWD(fn), ::std::as_const(v).value()); // side-effects only
    return FWD(v);
  }

  template <some_just V, typename Fn>
    requires(not ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>)
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(::std::as_const(v).value())>) -> V &&
    requires applicable_inspect<Fn &&, V &&>
  {
    ::fn::apply(FWD(fn), ::std::as_const(v).value()); // side-effects only
    return FWD(v);
  }

  template <some_just V, typename Fn>
    requires ::std::is_void_v<typename ::std::remove_cvref_t<V>::value_type>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(::fn::is_nothrow_applicable_v<Fn>) -> V &&
    requires applicable_inspect<Fn &&, V &&>
  {
    ::fn::apply(FWD(fn)); // side-effects only
    return FWD(v);
  }

  // An uninhabited value side never holds a value, so there is nothing to observe: the operand
  // passes through, and the callback is neither invoked nor instantiated.
  template <some_monadic_type V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&) const noexcept -> V &&
    requires some_empty_value<V>
  {
    return FWD(v);
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_INSPECT
