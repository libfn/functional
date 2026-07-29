// Copyright (c) 2024 Bronek Kozicki
//
// Distributed under the ISC License. See accompanying file LICENSE.md
// or copy at https://opensource.org/licenses/ISC

#ifndef INCLUDE_FN_INSPECT_ERROR
#define INCLUDE_FN_INSPECT_ERROR

#include <fn/functional.hpp>
#include <fn/functor.hpp>
#include <libfn_version.hpp>

#include <concepts>
#include <utility>

#include <fn/detail/macro_begin.hpp>

namespace fn {
inline namespace LIBFN_VERSION {
/**
 * @brief Checks if the monadic type can be used with the `inspect_error` operation
 *
 * @tparam Fn The function to observe the error
 * @tparam V The monadic type
 */
template <typename Fn, typename V>
concept applicable_inspect_error //
    = (some_expected<V> && requires(Fn &&fn, V &&v) {
        { ::fn::apply(FWD(fn), ::std::as_const(v).error()) } -> ::std::same_as<void>;
      }) || (some_optional<V> && requires(Fn &&fn) {
        { ::fn::apply(FWD(fn)) } -> ::std::same_as<void>;
      });

/**
 * @brief Observe the error for side-effects, passing the operand through unchanged
 *
 * The callback receives the error as const and must return `void`; on `optional` it is invoked
 * with no arguments - the empty state carries no error value. Rejected on `choice` and `just`,
 * which have no error side; on the identity `expected` it is vacuous - the operand passes through
 * and the callback is never instantiated.
 *
 * Use through the `fn::inspect_error` nielbloid.
 */
constexpr inline struct inspect_error_t final {
  /**
   * @brief Observe the error for side-effects, passing the operand through unchanged
   * @param fn The function to observe the error; invoked with no arguments on `optional`
   * @return A functor that will execute the function on the error
   */
  [[nodiscard]] constexpr auto operator()(auto &&fn) const
      noexcept(noexcept(functor<inspect_error_t, decltype(fn)>{FWD(fn)})) -> functor<inspect_error_t, decltype(fn)> //
  {
    return {FWD(fn)};
  }

  struct apply;
} inspect_error = {};

struct inspect_error_t::apply final {
  /**
   * @brief Observes the error for side-effects, when one is present
   *
   * @param v The monad
   * @param fn The function to observe the error
   * @return The operand, forwarded unchanged
   */
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const
      noexcept(::fn::is_nothrow_applicable_v<Fn, decltype(::std::as_const(v).error())>) -> V &&
    requires(not some_identity<V>) && applicable_inspect_error<Fn &&, V &&>
  {
    if (not v.has_value()) {
      ::fn::apply(FWD(fn), ::std::as_const(v).error()); // side-effects only
    }
    return FWD(v);
  }

  // An identity expected's error side is uninhabited - there is nothing to observe, the operand
  // passes through and the callback is never instantiated
  template <some_expected V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&) const noexcept -> V &&
    requires some_identity<V>
  {
    return FWD(v);
  }

  template <some_optional V, typename Fn>
  [[nodiscard]] constexpr auto operator()(V &&v, Fn &&fn) const noexcept(::fn::is_nothrow_applicable_v<Fn>) -> V &&
    requires applicable_inspect_error<Fn &&, V &&>
  {
    if (not v.has_value()) {
      ::fn::apply(FWD(fn)); // side-effects only
    }
    return FWD(v);
  }
};

} // namespace LIBFN_VERSION
} // namespace fn

#include <fn/detail/macro_end.hpp>

#endif // INCLUDE_FN_INSPECT_ERROR
